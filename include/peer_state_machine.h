#pragma once
#include <string>
#include <vector>
#include <winsock2.h>
#include "piece_manager.h"

class Peer {
public:
    Peer(const std::string& ip, uint16_t port, PieceManager* pm);

    void start();

private:
    std::string ip;
    uint16_t port;
    SOCKET sock;
    bool choked;
    PieceManager* pieceManager;

    bool receiveBitfield();
    void sendInterested();
    bool waitForUnchoke();
    void requestPiece(int index);
    bool receivePiece(int index);
};
