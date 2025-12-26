#pragma once
#include <vector>
#include <mutex>

class PieceManager {
public:
    PieceManager(int totalPieces);

    int getNextPiece();
    void markCompleted(int index);

private:
    std::vector<bool> completed;
    std::vector<bool> inProgress;
    std::mutex lock;
};
