#include "../include/tracker_http.h"
#include "../include/bencode.h"
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600  // Windows Vista or higher for getaddrinfo/freeaddrinfo
#endif

#include <winsock2.h>
#include <ws2tcpip.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <string>
#include <cstring>

// MinGW needs this
#ifdef __MINGW32__
typedef int socklen_t;
#endif
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

static std::string httpGET(const std::string& host, const std::string& path) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return "";
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return "";
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(80);
    serverAddr.sin_addr.s_addr = inet_addr(host.c_str()); // host must be IP address

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(sock);
        WSACleanup();
        return "";
    }

    std::string request =
        "GET " + path + " HTTP/1.1\r\n"
        "Host: " + host + "\r\n"
        "Connection: close\r\n\r\n";

    send(sock, request.c_str(), (int)request.size(), 0);

    char buffer[4096];
    std::string response;
    int bytes;

    while ((bytes = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        response.append(buffer, bytes);
    }

    closesocket(sock);
    WSACleanup();

    return response;
}


static std::string extractHTTPBody(const std::string& response) {
    size_t pos = response.find("\r\n\r\n");
    if (pos == std::string::npos) return "";
    return response.substr(pos + 4);
}

static std::vector<Peer> parseCompactPeers(const std::string& peers) {
    std::vector<Peer> result;

    for (size_t i = 0; i + 6 <= peers.size(); i += 6) {
        Peer p;
        p.ip = std::to_string((unsigned char)peers[i]) + "." +
               std::to_string((unsigned char)peers[i + 1]) + "." +
               std::to_string((unsigned char)peers[i + 2]) + "." +
               std::to_string((unsigned char)peers[i + 3]);
        p.port = ((unsigned char)peers[i + 4] << 8) |
                  (unsigned char)peers[i + 5];
        result.push_back(p);
    }

    return result;
}

std::vector<Peer> announceHTTP(
    const std::string& trackerURL,
    const std::string& infoHash,
    const std::string& peerId,
    long long left
) {
    // Remove "http://"
    std::string url = trackerURL.substr(7);
    size_t slashPos = url.find('/');
    if (slashPos == std::string::npos) return {};

    std::string host = url.substr(0, slashPos);
    std::string path = url.substr(slashPos);

    std::string query =
        path + "?info_hash=" + urlEncode(infoHash) +
        "&peer_id=" + urlEncode(peerId) +
        "&port=6881"
        "&uploaded=0"
        "&downloaded=0"
        "&left=" + std::to_string(left) +
        "&compact=1";

    std::string response = httpGET(host, query);
    std::string body = extractHTTPBody(response);

    if (body.empty()) return {};

    size_t pos = 0;
    BValue decoded = decodeValue(body, pos);
    if (!decoded.isDict()) return {};

    BDict dict = decoded.asDict();
    auto it = dict.find("peers");
    if (it == dict.end() || !it->second.isString()) return {};

    std::string peersBinary = it->second.asString();
    return parseCompactPeers(peersBinary);
}
