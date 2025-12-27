#pragma once
#include <string>
#include <winsock2.h>
#include "piece_manager.h"

class PeerStateMachine {
public:
    PeerStateMachine(
        const std::string& ip,
        int port,
        SOCKET sock,
        PieceManager* pieceManager
    );

    void start();

private:
    enum class State {
        HANDSHAKE,
        WAIT_BITFIELD,
        SEND_INTERESTED,
        WAIT_UNCHOKE,
        RUNNING,
        DEAD
    };

    std::string ip;
    int port;
    SOCKET sock;
    PieceManager* pieceManager;
    State state;

    bool recvAll(char* buf, int len);
    bool recvMessage(uint8_t& id, std::vector<uint8_t>& payload);
    bool sendInterested();
    bool handleMessage(uint8_t id, const std::vector<uint8_t>& payload);
};
