#include "../include/piece_manager.h"
#include "../include/sha1.h"
#include <cstring>

PieceManager::PieceManager(int totalPieces, int pieceLength)
    : totalPieces(totalPieces),
      pieceLength(pieceLength),
      pieceData(totalPieces),
      downloaded(totalPieces, 0) {}

int PieceManager::getNextPiece(const std::vector<bool>& peerBitfield) {
    std::lock_guard<std::mutex> lock(mtx);
    for (int i = 0; i < totalPieces; i++) {
        if (peerBitfield[i] && downloaded[i] == 0) {
            downloaded[i] = -1;
            pieceData[i].resize(pieceLength);
            return i;
        }
    }
    return -1;
}

void PieceManager::storeBlock(int piece, int offset, const std::vector<char>& data) {
    std::lock_guard<std::mutex> lock(mtx);
    memcpy(pieceData[piece].data() + offset, data.data(), data.size());
    downloaded[piece] += data.size();
}

bool PieceManager::isPieceComplete(int piece) {
    return downloaded[piece] >= pieceLength;
}

bool PieceManager::verifyPiece(int piece) {
    // Placeholder – you already have SHA1 logic
    return true;
}
