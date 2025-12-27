#pragma once

#include <cstdint>
#include <winsock2.h>
#include <string>
#include <vector>

SOCKET connectToPeer(
    const std::string& ip,
    uint16_t port,
    const std::vector<uint8_t>& handshake
);
