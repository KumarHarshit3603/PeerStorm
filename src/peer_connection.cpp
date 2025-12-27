#include "../include/peer_connection.h"
#include <ws2tcpip.h>

SOCKET connectToPeer(
    const std::string& ip,
    uint16_t port,
    const std::vector<uint8_t>& handshake
) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
        return INVALID_SOCKET;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
        closesocket(sock);
        return INVALID_SOCKET;
    }

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) != 0) {
        closesocket(sock);
        return INVALID_SOCKET;
    }

    // Send handshake
    int sent = send(sock,
                    reinterpret_cast<const char*>(handshake.data()),
                    handshake.size(),
                    0);

    if (sent != (int)handshake.size()) {
        closesocket(sock);
        return INVALID_SOCKET;
    }

    // Receive handshake
    uint8_t response[68];
    int received = recv(sock, reinterpret_cast<char*>(response), 68, 0);

    if (received != 68) {
        closesocket(sock);
        return INVALID_SOCKET;
    }

    return sock; // ✅ Fully connected & handshaken
}
