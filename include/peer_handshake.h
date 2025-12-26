#pragma once
#include <cstdint>
#include <string>
#include <vector>

std::vector<uint8_t> buildHandshake(
    const std::string& infoHash,
    const std::string& peerId
);
