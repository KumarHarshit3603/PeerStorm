#include "../include/piece_manager.h"

PieceManager::PieceManager(int totalPieces)
    : completed(totalPieces, false),
      inProgress(totalPieces, false) {}

int PieceManager::getNextPiece() {
    std::lock_guard<std::mutex> guard(lock);

    for (int i = 0; i < completed.size(); i++) {
        if (!completed[i] && !inProgress[i]) {
            inProgress[i] = true;
            return i;
        }
    }
    return -1;
}

void PieceManager::markCompleted(int index) {
    std::lock_guard<std::mutex> guard(lock);
    completed[index] = true;
    inProgress[index] = false;
}

