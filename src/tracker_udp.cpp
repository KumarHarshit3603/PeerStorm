#define _WIN32_WINNT 0x0600

#include "../include/tracker_udp.h"

#include <winsock2.h>
#include <ws2tcpip.h>

#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

// ------------------ helpers ------------------
static uint64_t htonll(uint64_t x) {
    return (static_cast<uint64_t>(htonl(uint32_t(x & 0xFFFFFFFFULL))) << 32) |
           htonl(uint32_t(x >> 32));
}

static uint64_t ntohll(uint64_t x) {
    return (static_cast<uint64_t>(ntohl(uint32_t(x & 0xFFFFFFFFULL))) << 32) |
           ntohl(uint32_t(x >> 32));
}

// ------------------ UDP tracker ------------------
std::vector<Peer> announceUDP(
    const std::string& trackerURL,
    const std::string& infoHash,
    const std::string& peerId,
    long long left
) {
    std::vector<Peer> peers;

    std::cout << "\n[UDP] Tracker: " << trackerURL << "\n";

    // ---- Parse URL ----
    if (trackerURL.rfind("udp://", 0) != 0) {
        std::cout << "[UDP] Invalid scheme\n";
        return peers;
    }

    std::string url = trackerURL.substr(6);
    size_t colon = url.find(':');
    size_t slash = url.find('/');

    if (colon == std::string::npos) {
        std::cout << "[UDP] URL parse failed\n";
        return peers;
    }

    std::string host = url.substr(0, colon);
    std::string portStr =
        (slash == std::string::npos)
            ? url.substr(colon + 1)
            : url.substr(colon + 1, slash - colon - 1);

    std::cout << "[UDP] Host: " << host << "  Port: " << portStr << "\n";

    // ---- Winsock ----
    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cout << "[UDP] WSAStartup failed\n";
        return peers;
    }

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        std::cout << "[UDP] socket() failed\n";
        WSACleanup();
        return peers;
    }

    // ---- timeout ----
    DWORD timeoutMs = 3000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
               (const char*)&timeoutMs, sizeof(timeoutMs));

    // ---- resolve ----
    addrinfo hints{}, *res = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res) != 0) {
        std::cout << "[UDP] DNS resolution failed\n";
        closesocket(sock);
        WSACleanup();
        return peers;
    }

    sockaddr_in addr = *(sockaddr_in*)res->ai_addr;
    freeaddrinfo(res);

    // ================= CONNECT =================
    std::cout << "[UDP] Sending CONNECT...\n";

    uint8_t connectReq[16];
    uint64_t protocolId = htonll(0x41727101980ULL);
    uint32_t actionNet = htonl(0);

    uint32_t connectTx = rand();
    uint32_t connectTxNet = htonl(connectTx);

    memcpy(connectReq, &protocolId, 8);
    memcpy(connectReq + 8, &actionNet, 4);
    memcpy(connectReq + 12, &connectTxNet, 4);

    sendto(sock, (char*)connectReq, 16, 0,
           (sockaddr*)&addr, sizeof(addr));

    uint8_t connectResp[16];
    int len = recv(sock, (char*)connectResp, 16, 0);
    if (len != 16) {
        std::cout << "[UDP] CONNECT timeout / invalid response\n";
        closesocket(sock);
        WSACleanup();
        return peers;
    }

    uint32_t respActionNet, respTxNet;
    memcpy(&respActionNet, connectResp, 4);
    memcpy(&respTxNet, connectResp + 4, 4);

    if (ntohl(respActionNet) != 0 || ntohl(respTxNet) != connectTx) {
        std::cout << "[UDP] CONNECT rejected (tx/action mismatch)\n";
        closesocket(sock);
        WSACleanup();
        return peers;
    }

    uint64_t connectionIdNet;
    memcpy(&connectionIdNet, connectResp + 8, 8);
    uint64_t connectionId = ntohll(connectionIdNet);

    std::cout << "[UDP] CONNECT OK, connectionId=" << connectionId << "\n";

    // ================= ANNOUNCE =================
    std::cout << "[UDP] Sending ANNOUNCE...\n";

    uint8_t announceReq[98]{};

    uint32_t announceActionNet = htonl(1);
    uint32_t announceTx = rand();
    uint32_t announceTxNet = htonl(announceTx);

    uint64_t connectionIdSend = htonll(connectionId);

    memcpy(announceReq, &connectionIdSend, 8);
    memcpy(announceReq + 8, &announceActionNet, 4);
    memcpy(announceReq + 12, &announceTxNet, 4);

    memcpy(announceReq + 16, infoHash.data(), 20);
    memcpy(announceReq + 36, peerId.data(), 20);

    uint64_t downloaded = htonll(0);
    uint64_t leftNet = htonll(left);
    uint64_t uploaded = htonll(0);

    memcpy(announceReq + 56, &downloaded, 8);
    memcpy(announceReq + 64, &leftNet, 8);
    memcpy(announceReq + 72, &uploaded, 8);

    uint32_t eventNet = htonl(0);
    uint32_t ipNet = htonl(0);
    uint32_t keyNet = htonl(rand());
    int32_t numWantNet = htonl(-1);
    uint16_t portNet = htons(6881);

    memcpy(announceReq + 80, &eventNet, 4);
    memcpy(announceReq + 84, &ipNet, 4);
    memcpy(announceReq + 88, &keyNet, 4);
    memcpy(announceReq + 92, &numWantNet, 4);
    memcpy(announceReq + 96, &portNet, 2);

    sendto(sock, (char*)announceReq, 98, 0,
           (sockaddr*)&addr, sizeof(addr));

    uint8_t resp[1500];
    len = recv(sock, (char*)resp, sizeof(resp), 0);
    if (len < 20) {
        std::cout << "[UDP] ANNOUNCE timeout\n";
        closesocket(sock);
        WSACleanup();
        return peers;
    }

    uint32_t respAction2Net, respTx2Net;
    memcpy(&respAction2Net, resp, 4);
    memcpy(&respTx2Net, resp + 4, 4);

    uint32_t respAction2 = ntohl(respAction2Net);
    uint32_t respTx2 = ntohl(respTx2Net);

    if (respAction2 == 3) {
        std::string err((char*)resp + 8, len - 8);
        std::cout << "[UDP] Tracker ERROR: " << err << "\n";
        closesocket(sock);
        WSACleanup();
        return peers;
    }

    if (respAction2 != 1 || respTx2 != announceTx) {
        std::cout << "[UDP] ANNOUNCE rejected (tx/action mismatch)\n";
        closesocket(sock);
        WSACleanup();
        return peers;
    }

    int peerCount = (len - 20) / 6;
    std::cout << "[UDP] ANNOUNCE OK — peers=" << peerCount << "\n";

    // ---- peers ----
    for (int i = 20; i + 6 <= len; i += 6) {
        Peer p;
        p.ip =
            std::to_string(resp[i]) + "." +
            std::to_string(resp[i + 1]) + "." +
            std::to_string(resp[i + 2]) + "." +
            std::to_string(resp[i + 3]);
        p.port = (resp[i + 4] << 8) | resp[i + 5];
        peers.push_back(p);
    }

    closesocket(sock);
    WSACleanup();
    return peers;
}
