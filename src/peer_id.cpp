#include "../include/peer_id.h"

#include <random>
#include <sstream>
#include <iomanip>

std::string generatePeerId() {
    // Client ID: PS = PeerStorm
    // Version: 0100 = v1.0.0
    std::string prefix = "-PS0100-";  // 8 bytes

    // Remaining bytes = 20 - 8 = 12
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 15);

    std::ostringstream suffix;
    for (int i = 0; i < 12; i++) {
        suffix << std::hex << dist(gen);
    }

    return prefix + suffix.str();  // exactly 20 bytes
}
