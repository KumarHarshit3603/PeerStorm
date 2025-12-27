#include "../include/peer_state_machine.h"
#include <vector>
#include <iostream>
#include <mutex>
#include <cstdint>

static std::mutex logMutex;

PeerStateMachine::PeerStateMachine(
    const std::string& ip, int port,
    SOCKET sock, PieceManager* pm
)
    : ip(ip), port(port), sock(sock), pieceManager(pm) {}

bool PeerStateMachine::recvAll(char* buf, int len) {
    int total = 0;
    while (total < len) {
        int r = recv(sock, buf + total, len - total, 0);
        if (r <= 0) return false;
        total += r;
    }
    return true;
}

bool PeerStateMachine::recvMessage(
        uint8_t &id,
        std::vector<uint8_t> &payload) {
    uint32_t len;
    if(!recvAll((char*)&len, 4)) return false;
    len = ntohl(len);

    if (len == 0) {
        id = 255;
        payload.clear();
        return true;
    }
    recvAll((char*)&id, 1);
    payload.resize(len-1);
    if(len>1) recvAll((char*)payload.data(), payload.size());
    return true;
}

bool PeerStateMachine::sendInterested() {
    uint32_t len = htonl(1);
    uint8_t id = 2;
    send(sock, (char*)&len, 4, 0);
    send(sock, (char*)&id, 1, 0);
    return true;
}

bool PeerStateMachine::sendRequest(
        int piece, int offset, int length) {
    uint32_t len = htonl(13);
    uint8_t id = 6;
    uint8_t buf[17];
    memcpy(buf+0, &id, 1);
    memcpy(buf+1, &piece, 4);
    memcpy(buf+5, &offset,4);
    memcpy(buf+9, &length,4);

    send(sock, (char*)&len, 4, 0);
    send(sock, (char*)buf, 13, 0);
    return true;
}

void PeerStateMachine::start() {
    int timeout = 8000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
               (char*)&timeout, sizeof(timeout));

    uint8_t id;
    std::vector<uint8_t> payload;

    bool unchoked = false;
    bool interestedSent = false;

    while (true) {
        if (!recvMessage(id, payload)) break;

        if (id == 5 && !interestedSent) {
            sendInterested();
            interestedSent = true;
            continue;
        }

        if (id == 1) {
            unchoked = true;
        }

        if (unchoked) {
            // Request next block
            BlockRequest br;
            if (pieceManager->getNextBlock(br)) {
                sendRequest(br.pieceIndex,
                            br.offset,
                            br.length);

                // Wait for piece message
                uint8_t recvId;
                std::vector<uint8_t> recvPayload;
                if(!recvMessage(recvId, recvPayload))
                    break;

                if (recvId == 7) {
                    // piece message
                    int idx = br.pieceIndex;
                    int off = br.offset;
                    pieceManager->storeBlock(idx, off, recvPayload);

                    // check if piece done
                    // (simple count logic)
                    // assume only one block per piece for simplicity
                    pieceManager->markCompleted(idx);

                    {
                        std::lock_guard<std::mutex> lk(logMutex);
                        std::cout << "[PIECE] "
                                  << idx << " from "
                                  << ip << "\n";
                    }
                }
            }
        }
    }

    {
        std::lock_guard<std::mutex> lk(logMutex);
        std::cout << "[DISCONNECTED] " << ip << " : " << port << "\n";
    }
}
