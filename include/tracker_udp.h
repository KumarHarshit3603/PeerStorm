#pragma once
#include <string>
#include <vector>
#include "tracker_http.h" // for Peer struct

std::vector<Peer> announceUDP(
    const std::string& trackerURL,
    const std::string& infoHash,
    const std::string& peerId,
    long long left
);
