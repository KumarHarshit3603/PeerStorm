#pragma once

#include <winsock2.h>
#include <string>
#include "piece_manager.h"

class PeerStateMachine {
public:
    PeerStateMachine(
        const std::string& ip,
        uint16_t port,
        SOCKET sock,
        PieceManager* pm
    );

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
