#pragma once
#include <winsock2.h>
#include <string>
#include "piece_manager.h"

class PeerStateMachine {
public:
    PeerStateMachine(
        const std::string& ip,
        int port,
        SOCKET sock,
        PieceManager* pm
    );

    void start();

private:
    bool recvAll(char* buf, int len);
    bool recvMessage(uint8_t &id, std::vector<uint8_t> &payload);
    bool sendInterested();
    bool sendRequest(int piece, int offset, int length);

    std::string ip;
    int port;
    SOCKET sock;
    PieceManager* pieceManager;
};
