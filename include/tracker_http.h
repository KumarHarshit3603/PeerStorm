#ifndef TRACKER_HTTP_H
#define TRACKER_HTTP_H

#include <string>
#include <vector>
#include "peer.h"

std::vector<Peer> announceHTTP(
    const std::string& trackerURL,
    const std::string& infoHash,
    const std::string& peerId,
    long long left
);

#endif
