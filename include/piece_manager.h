#pragma once
#include <vector>
#include <mutex>
#include <string>

class PieceManager {
public:
    PieceManager(int totalPieces, int pieceLength);

    int getNextPiece(const std::vector<bool>& peerBitfield);
    void storeBlock(int piece, int offset, const std::vector<char>& data);

    bool isPieceComplete(int piece);
    bool verifyPiece(int piece);

private:
    int totalPieces;
    int pieceLength;

    std::vector<std::vector<char>> pieceData;
    std::vector<int> downloaded;
    std::mutex mtx;
};
