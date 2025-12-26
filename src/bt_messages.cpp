#include "../include/bt_messages.h"

BTMessage parseMessage(const std::vector<uint8_t>& buffer) {
    if (buffer.size() < 4) {
        return {BTMessageType::UNKNOWN, {}};
    }

    uint32_t length =
        (buffer[0] << 24) |
        (buffer[1] << 16) |
        (buffer[2] << 8)  |
        buffer[3];

    if (length == 0) {
        return {BTMessageType::KEEPALIVE, {}};
    }

    if (buffer.size() < length + 4) {
        return {BTMessageType::UNKNOWN, {}};
    }

    BTMessageType type = static_cast<BTMessageType>(buffer[4]);
    std::vector<uint8_t> payload(
        buffer.begin() + 5,
        buffer.begin() + 4 + length
    );

    return {type, payload};
}
