#include "peer_state_machine.h"
#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <mutex>

static std::mutex logMutex;

PeerStateMachine::PeerStateMachine(
    const std::string& ip,
    int port,
    SOCKET sock,
    PieceManager* pieceManager
)
    : ip(ip),
      port(port),
      sock(sock),
      pieceManager(pieceManager),
      state(State::HANDSHAKE) {}

bool PeerStateMachine::recvAll(char* buf, int len) {
    int received = 0;
    while (received < len) {
        int r = recv(sock, buf + received, len - received, 0);
        if (r <= 0) return false;
        received += r;
    }
    return true;
}

bool PeerStateMachine::recvMessage(uint8_t& id, std::vector<uint8_t>& payload) {
    uint32_t len;
    if (!recvAll(reinterpret_cast<char*>(&len), 4))
        return false;

    len = ntohl(len);

    if (len == 0) {
        id = 255; // keep-alive
        payload.clear();
        return true;
    }

    if (!recvAll(reinterpret_cast<char*>(&id), 1))
        return false;

    payload.resize(len - 1);
    if (!payload.empty()) {
        if (!recvAll(reinterpret_cast<char*>(payload.data()), payload.size()))
            return false;
    }
    return true;
}

bool PeerStateMachine::sendInterested() {
    uint32_t len = htonl(1);
    uint8_t id = 2; // interested

    if (send(sock, reinterpret_cast<char*>(&len), 4, 0) <= 0)
        return false;
    if (send(sock, reinterpret_cast<char*>(&id), 1, 0) <= 0)
        return false;

    return true;
}

bool PeerStateMachine::handleMessage(uint8_t id, const std::vector<uint8_t>& payload) {
    switch (id) {
        case 0: // choke
            return true;
        case 1: // unchoke
            state = State::RUNNING;
            {
                std::lock_guard<std::mutex> lock(logMutex);
                std::cout << "[UNCHOKED] " << ip << ":" << port << "\n";
            }
            return true;
        case 5: // bitfield
            {
                std::lock_guard<std::mutex> lock(logMutex);
                std::cout << "[BITFIELD] " << ip << ":" << port
                          << " (" << payload.size() << " bytes)\n";
            }
            return true;
        case 255: // keep-alive
            return true;
        default:
            return true;
    }
}

void PeerStateMachine::start() {
    // --- Socket timeout (CRITICAL FIX)
    int timeout = 8000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
               reinterpret_cast<char*>(&timeout), sizeof(timeout));

    state = State::WAIT_BITFIELD;

    while (state != State::DEAD) {
        uint8_t id;
        std::vector<uint8_t> payload;

        if (!recvMessage(id, payload)) {
            std::lock_guard<std::mutex> lock(logMutex);
            std::cout << "[DISCONNECTED] " << ip << ":" << port << "\n";
            break;
        }

        if (state == State::WAIT_BITFIELD && id == 5) {
            handleMessage(id, payload);
            sendInterested();
            state = State::WAIT_UNCHOKE;
            continue;
        }

        if (state == State::WAIT_UNCHOKE && id == 1) {
            handleMessage(id, payload);
            continue;
        }

        handleMessage(id, payload);
    }
}
