#define _WIN32_WINNT 0x0600  // MUST be before winsock includes

#include "../include/tracker_http.h"
#include "../include/bencode.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>
#include <cstring>
#include <cctype>

#pragma comment(lib, "ws2_32.lib")

// ---------------- URL encode ----------------
static std::string urlEncode(const std::string& data) {
    std::ostringstream encoded;
    for (unsigned char c : data) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded << c;
        } else {
            encoded << '%' << std::uppercase << std::hex
                    << std::setw(2) << std::setfill('0')
                    << (int)c;
        }
    }
    return encoded.str();
}

// ---------------- HTTP GET ----------------
static std::string httpGET(const std::string& host, const std::string& path) {
    std::cout << "[HTTP] Resolving host: " << host << "\n";

    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cout << "[HTTP] WSAStartup failed\n";
        return "";
    }

    addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host.c_str(), "80", &hints, &res) != 0) {
        std::cout << "[HTTP] DNS resolution failed\n";
        WSACleanup();
        return "";
    }

    SOCKET sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == INVALID_SOCKET) {
        std::cout << "[HTTP] socket() failed\n";
        freeaddrinfo(res);
        WSACleanup();
        return "";
    }

    std::cout << "[HTTP] Connecting...\n";
    if (connect(sock, res->ai_addr, (int)res->ai_addrlen) == SOCKET_ERROR) {
        std::cout << "[HTTP] connect() failed\n";
        closesocket(sock);
        freeaddrinfo(res);
        WSACleanup();
        return "";
    }

    freeaddrinfo(res);

    std::string request =
        "GET " + path + " HTTP/1.1\r\n"
        "Host: " + host + "\r\n"
        "User-Agent: PeerStorm/0.1\r\n"
        "Connection: close\r\n\r\n";

    std::cout << "[HTTP] Sending request\n";
    send(sock, request.c_str(), (int)request.size(), 0);

    char buffer[4096];
    std::string response;
    int bytes = 0;

    while ((bytes = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        response.append(buffer, bytes);
    }

    closesocket(sock);
    WSACleanup();

    std::cout << "[HTTP] Response received (" << response.size() << " bytes)\n";
    return response;
}

// ---------------- Extract body ----------------
static std::string extractHTTPBody(const std::string& response) {
    size_t pos = response.find("\r\n\r\n");
    if (pos == std::string::npos) return "";
    return response.substr(pos + 4);
}

// ---------------- Parse compact peers ----------------
static std::vector<Peer> parseCompactPeers(const std::string& peers) {
    std::vector<Peer> result;

    for (size_t i = 0; i + 6 <= peers.size(); i += 6) {
        Peer p;
        p.ip =
            std::to_string((unsigned char)peers[i]) + "." +
            std::to_string((unsigned char)peers[i + 1]) + "." +
            std::to_string((unsigned char)peers[i + 2]) + "." +
            std::to_string((unsigned char)peers[i + 3]);

        p.port =
            ((unsigned char)peers[i + 4] << 8) |
             (unsigned char)peers[i + 5];

        result.push_back(p);
    }

    return result;
}

// ---------------- HTTP announce ----------------
std::vector<Peer> announceHTTP(
    const std::string& trackerURL,
    const std::string& infoHash,
    const std::string& peerId,
    long long left
) {
    std::vector<Peer> peers;

    std::cout << "\n[HTTP] Tracker: " << trackerURL << "\n";

    if (trackerURL.rfind("http://", 0) != 0) {
        std::cout << "[HTTP] Invalid scheme\n";
        return peers;
    }

    std::string url = trackerURL.substr(7);
    size_t slash = url.find('/');
    if (slash == std::string::npos) {
        std::cout << "[HTTP] URL parse failed\n";
        return peers;
    }

    std::string host = url.substr(0, slash);
    std::string path = url.substr(slash);

    std::string query =
        path + "?info_hash=" + urlEncode(infoHash) +
        "&peer_id=" + urlEncode(peerId) +
        "&port=6881"
        "&uploaded=0"
        "&downloaded=0"
        "&left=" + std::to_string(left) +
        "&compact=1";

    std::cout << "[HTTP] Full URL:\n";
    std::cout << "http://" << host << query << "\n";

    std::string response = httpGET(host, query);
    if (response.empty()) {
        std::cout << "[HTTP] Empty response\n";
        return peers;
    }

    std::string body = extractHTTPBody(response);
    if (body.empty()) {
        std::cout << "[HTTP] No HTTP body\n";
        return peers;
    }

    size_t pos = 0;
    BValue decoded = decodeValue(body, pos);

    if (!decoded.isDict()) {
        std::cout << "[HTTP] Invalid bencode\n";
        return peers;
    }

    BDict dict = decoded.asDict();

    auto itFail = dict.find("failure reason");
    if (itFail != dict.end()) {
        std::cout << "[HTTP] Tracker failure: "
                  << itFail->second.asString() << "\n";
        return peers;
    }

    auto itPeers = dict.find("peers");
    if (itPeers == dict.end() || !itPeers->second.isString()) {
        std::cout << "[HTTP] No peers field\n";
        return peers;
    }

    peers = parseCompactPeers(itPeers->second.asString());
    std::cout << "[HTTP] Peers received: " << peers.size() << "\n";

    return peers;
}
