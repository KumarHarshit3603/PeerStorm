#pragma once
#include <vector>
#include <mutex>
#include <set>
#include <unordered_map>

struct BlockRequest {
    int pieceIndex;
    int offset;
    int length;
};

class PieceManager {
public:
    PieceManager(int totalPieces, int pieceLength);

    // Called by worker to pick next block request
    bool getNextBlock(BlockRequest &req);

    // Called when a block arrives
    void storeBlock(int pieceIndex, int offset,
                    const std::vector<uint8_t>& data);

    // Called when piece is complete
    void markCompleted(int pieceIndex);

    int getPieceLength(int pieceIndex) const;
    int getTotalPieces() const;

private:
    int totalPieces;
    int standardBlockSize;

    std::mutex lock;
    std::set<int> completedPieces;

    // For each piece, track received blocks
    std::unordered_map<int, std::vector<uint8_t>> pieceBuffers;
    std::unordered_map<int, std::set<int>> receivedOffsets;
};
