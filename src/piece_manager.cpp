#include "../include/piece_manager.h"
#include <cstring>

PieceManager::PieceManager(int total, int pieceLength)
    : totalPieces(total), standardBlockSize(16384) {}

bool PieceManager::getNextBlock(BlockRequest& req) {
    std::lock_guard<std::mutex> guard(lock);

    for (int p = 0; p < totalPieces; p++) {
        if (completedPieces.count(p)) continue;

        int pieceSize = getPieceLength(p);
        int blocks = (pieceSize + standardBlockSize - 1)
                     / standardBlockSize;

        for (int b = 0; b < blocks; b++) {
            int offset = b * standardBlockSize;
            if (!receivedOffsets[p].count(offset)) {
                req.pieceIndex = p;
                req.offset = offset;
                req.length = std::min(standardBlockSize,
                                      pieceSize - offset);
                return true;
            }
        }
    }
    return false;
}

void PieceManager::storeBlock(
        int pieceIndex, int offset,
        const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> guard(lock);
    auto &buf = pieceBuffers[pieceIndex];
    if (buf.size() < (size_t)offset + data.size())
        buf.resize(offset + data.size());
    memcpy(buf.data() + offset, data.data(), data.size());
    receivedOffsets[pieceIndex].insert(offset);
}

void PieceManager::markCompleted(int pieceIndex) {
    std::lock_guard<std::mutex> guard(lock);
    completedPieces.insert(pieceIndex);
    pieceBuffers.erase(pieceIndex);
    receivedOffsets.erase(pieceIndex);
}

int PieceManager::getTotalPieces() const {
    return totalPieces;
}

int PieceManager::getPieceLength(int pieceIndex) const {
    // All pieces except last are equal
    // Last piece size logic needed, but for simplicity:
    return standardBlockSize * 16;
}
