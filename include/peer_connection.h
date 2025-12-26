#pragma once
#include <cstdint>
#include <string>
#include <vector>

bool connectToPeer(
    const std::string& ip,
    uint16_t port,
    const std::vector<uint8_t>& handshake
);
