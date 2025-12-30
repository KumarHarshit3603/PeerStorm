#pragma once
#include <string>
#include <vector>
#include <winsock2.h>
#include "piece_manager.h"

class PeerStateMachine {
public:
    PeerStateMachine(
        const std::string& ip,
        int port,
        const std::string& infoHash,
        const std::string& peerId,
        PieceManager& pieceManager
    );

    void run();

private:
    std::string ip;
    int port;
    std::string infoHash;
    std::string peerId;

    SOCKET sock;
    PieceManager& pieceManager;

    std::vector<bool> peerBitfield;

    bool connectToPeer();
    bool performHandshake();
    bool receiveHandshake();

    bool sendInterested();
    bool sendRequest(int pieceIndex, int offset, int length);

    bool receiveLoop();
    bool handleMessage(uint8_t id, const std::vector<char>& payload);

    bool recvExact(char* buffer, int length);
    bool sendAll(const char* buffer, int length);

    void closeConnection();
};
