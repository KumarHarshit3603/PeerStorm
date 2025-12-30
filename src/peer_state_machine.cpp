#include "../include/peer_state_machine.h"
#include "../include/sha1.h"
#include <iostream>
#include <cstring>


#define BLOCK_SIZE 16384

PeerStateMachine::PeerStateMachine(
    const std::string& ip,
    int port,
    const std::string& infoHash,
    const std::string& peerId,
    PieceManager& pieceManager
)
    : ip(ip),
      port(port),
      infoHash(infoHash),
      peerId(peerId),
      pieceManager(pieceManager),
      sock(INVALID_SOCKET) {}

void PeerStateMachine::run() {
    if (!connectToPeer()) return;
    if (!performHandshake()) return;
    if (!receiveHandshake()) return;
    if (!sendInterested()) return;

    receiveLoop();
    closeConnection();
}

bool PeerStateMachine::connectToPeer() {
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) return false;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(sock);
        return false;
    }

    std::cout << "[+] Connected: " << ip << ":" << port << "\n";
    return true;
}

bool PeerStateMachine::performHandshake() {
    char handshake[68] = {0};
    handshake[0] = 19;
    memcpy(handshake + 1, "BitTorrent protocol", 19);
    memcpy(handshake + 28, infoHash.data(), 20);
    memcpy(handshake + 48, peerId.data(), 20);

    return sendAll(handshake, 68);
}

bool PeerStateMachine::receiveHandshake() {
    char response[68];
    if (!recvExact(response, 68)) return false;

    if (memcmp(response + 28, infoHash.data(), 20) != 0) {
        std::cerr << "[!] Info hash mismatch\n";
        return false;
    }
    return true;
}

bool PeerStateMachine::sendInterested() {
    uint32_t len = htonl(1);
    uint8_t id = 2;

    char msg[5];
    memcpy(msg, &len, 4);
    msg[4] = id;

    return sendAll(msg, 5);
}

bool PeerStateMachine::receiveLoop() {
    while (true) {
        uint32_t lenNet;
        if (!recvExact((char*)&lenNet, 4)) return false;

        uint32_t len = ntohl(lenNet);
        if (len == 0) continue;

        uint8_t id;
        if (!recvExact((char*)&id, 1)) return false;

        std::vector<char> payload(len - 1);
        if (len > 1 && !recvExact(payload.data(), len - 1)) return false;

        if (!handleMessage(id, payload)) return false;
    }
}

bool PeerStateMachine::handleMessage(uint8_t id, const std::vector<char>& payload) {
    switch (id) {
        case 1: { // unchoke
            int piece = pieceManager.getNextPiece(peerBitfield);
            if (piece >= 0) {
                sendRequest(piece, 0, BLOCK_SIZE);
            }
            break;
        }
        case 5: { // bitfield
            peerBitfield.resize(payload.size() * 8);
            for (size_t i = 0; i < payload.size(); i++) {
                for (int b = 0; b < 8; b++) {
                    peerBitfield[i * 8 + b] = payload[i] & (1 << (7 - b));
                }
            }
            break;
        }
        case 7: { // piece
            int index = ntohl(*(int*)&payload[0]);
            int begin = ntohl(*(int*)&payload[4]);

            std::vector<char> block(payload.begin() + 8, payload.end());
            pieceManager.storeBlock(index, begin, block);

            if (pieceManager.isPieceComplete(index)) {
                if (pieceManager.verifyPiece(index)) {
                    std::cout << "[✔] Piece verified: " << index << "\n";
                }
            }
            break;
        }
        default:
            break;
    }
    return true;
}

bool PeerStateMachine::sendRequest(int pieceIndex, int offset, int length) {
    char msg[17];
    uint32_t len = htonl(13);
    uint8_t id = 6;

    memcpy(msg, &len, 4);
    msg[4] = id;
    *(int*)(msg + 5) = htonl(pieceIndex);
    *(int*)(msg + 9) = htonl(offset);
    *(int*)(msg + 13) = htonl(length);

    return sendAll(msg, 17);
}

bool PeerStateMachine::recvExact(char* buffer, int length) {
    int received = 0;
    while (received < length) {
        int r = recv(sock, buffer + received, length - received, 0);
        if (r <= 0) return false;
        received += r;
    }
    return true;
}

bool PeerStateMachine::sendAll(const char* buffer, int length) {
    int sent = 0;
    while (sent < length) {
        int s = send(sock, buffer + sent, length - sent, 0);
        if (s <= 0) return false;
        sent += s;
    }
    return true;
}

void PeerStateMachine::closeConnection() {
    closesocket(sock);
    std::cout << "[DISCONNECTED] " << ip << ":" << port << "\n";
}
