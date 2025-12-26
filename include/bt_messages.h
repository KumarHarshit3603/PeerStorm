#pragma once
#include <cstdint>
#include <vector>

enum class BTMessageType {
    CHOKE = 0,
    UNCHOKE = 1,
    INTERESTED = 2,
    NOT_INTERESTED = 3,
    HAVE = 4,
    BITFIELD = 5,
    REQUEST = 6,
    PIECE = 7,
    CANCEL = 8,
    KEEPALIVE = -1,
    UNKNOWN = -2
};

struct BTMessage {
    BTMessageType type;
    std::vector<uint8_t> payload;
};

BTMessage parseMessage(const std::vector<uint8_t>& buffer);
