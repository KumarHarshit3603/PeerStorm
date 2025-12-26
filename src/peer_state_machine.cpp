#include "../include/peer_state_machine.h"
#include "../include/bt_messages.h"
#include <iostream>

Peer::Peer(const std::string& ip, uint16_t port, PieceManager* pm)
    : ip(ip), port(port), sock(INVALID_SOCKET),
      choked(true), pieceManager(pm) {}

void Peer::start() {
    // socket already created + handshake done earlier
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

bool Peer::receiveBitfield() {
    uint8_t buf[4096];
    int r = recv(sock, (char*)buf, sizeof(buf), 0);
    if (r <= 0) return false;

    std::vector<uint8_t> data(buf, buf + r);
    BTMessage msg = parseMessage(data);

    return msg.type == BTMessageType::BITFIELD;
}

void Peer::sendInterested() {
    uint8_t msg[5] = {0,0,0,1,2};
    send(sock, (char*)msg, 5, 0);
}

bool Peer::waitForUnchoke() {
    uint8_t buf[1024];
    int r = recv(sock, (char*)buf, sizeof(buf), 0);
    if (r <= 0) return false;

    std::vector<uint8_t> data(buf, buf + r);
    BTMessage msg = parseMessage(data);

    if (msg.type == BTMessageType::UNCHOKE) {
        choked = false;
        return true;
    }
    return false;
}

void Peer::requestPiece(int index) {
    uint8_t msg[17] = {0,0,0,13,6};
    msg[5] = (index >> 24) & 0xFF;
    msg[6] = (index >> 16) & 0xFF;
    msg[7] = (index >> 8) & 0xFF;
    msg[8] = index & 0xFF;
    send(sock, (char*)msg, 17, 0);
}

bool Peer::receivePiece(int index) {
    uint8_t buf[16384];
    int r = recv(sock, (char*)buf, sizeof(buf), 0);
    if (r <= 0) return false;

    std::cout << "Received piece " << index
              << " from " << ip << "\n";
    return true;
}
