#include "../include/peer_state_machine.h"
#include "../include/bt_messages.h"
#include <vector>

PeerStateMachine::PeerStateMachine(
    const std::string& ip,
    uint16_t port,
    SOCKET sock,
    PieceManager* pm
) : ip(ip),
    port(port),
    sock(sock),
    choked(true),
    pieceManager(pm) {}

void PeerStateMachine::start() {
    if (!receiveBitfield()) return;

    sendInterested();

    if (!waitForUnchoke()) return;

    while (true) {
        int piece = pieceManager->getNextPiece();
        if (piece == -1) break;

        requestPiece(piece);
        if (receivePiece(piece)) {
            pieceManager->markCompleted(piece);
        }
    }
}

bool PeerStateMachine::receiveBitfield() {
    uint8_t buf[4096];
    int r = recv(sock, (char*)buf, sizeof(buf), 0);
    if (r <= 0) return false;

    auto msg = parseMessage({buf, buf + r});
    return msg.type == BTMessageType::BITFIELD;
}

void PeerStateMachine::sendInterested() {
    uint8_t msg[5] = {0, 0, 0, 1, 2};
    send(sock, (char*)msg, 5, 0);
}

bool PeerStateMachine::waitForUnchoke() {
    uint8_t buf[1024];
    int r = recv(sock, (char*)buf, sizeof(buf), 0);
    if (r <= 0) return false;

    auto msg = parseMessage({buf, buf + r});
    if (msg.type == BTMessageType::UNCHOKE) {
        choked = false;
        return true;
    }
    return false;
}

void PeerStateMachine::requestPiece(int index) {
    uint8_t msg[17] = {0,0,0,13,6};
    msg[5] = (index >> 24) & 0xFF;
    msg[6] = (index >> 16) & 0xFF;
    msg[7] = (index >> 8) & 0xFF;
    msg[8] = index & 0xFF;
    send(sock, (char*)msg, 17, 0);
}

bool PeerStateMachine::receivePiece(int index) {
    uint8_t buf[16384];
    int r = recv(sock, (char*)buf, sizeof(buf), 0);
    return r > 0;
}
