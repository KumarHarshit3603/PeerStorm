#ifndef PEER_H
#define PEER_H

#include <string>
#include <cstdint>

struct Peer {
    std::string ip;
    uint16_t port;
};

#endif
