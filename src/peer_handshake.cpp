#include "../include/peer_handshake.h"

std::vector<uint8_t> buildHandshake(
    const std::string& infoHash,
    const std::string& peerId
) {
    std::vector<uint8_t> buf;

    // pstrlen
    buf.push_back(19);

    // protocol string
    const std::string proto = "BitTorrent protocol";
    buf.insert(buf.end(), proto.begin(), proto.end());

    // reserved bytes
    buf.insert(buf.end(), 8, 0);

    // info_hash (20 bytes)
    buf.insert(buf.end(), infoHash.begin(), infoHash.end());

    // peer_id (20 bytes)
    buf.insert(buf.end(), peerId.begin(), peerId.end());

    return buf;
}
