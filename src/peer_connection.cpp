#include "../include/peer_connection.h"
#include <iostream>

#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

bool connectToPeer(
    const std::string& ip,
    uint16_t port,
    const std::vector<uint8_t>& handshake
) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed. WSA error: "
                  << WSAGetLastError() << "\n";
        return false;
    }


    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    if (addr.sin_addr.s_addr == INADDR_NONE) {
        std::cerr << "Invalid IP address: " << ip << "\n";
        closesocket(sock);
        return false;
    }

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed to " << ip << ":" << port << "\n";
        closesocket(sock);
        return false;
    }

    send(sock, (const char*)handshake.data(), (int)handshake.size(), 0);

    uint8_t response[68];
    int received = recv(sock, (char*)response, 68, 0);

    if (received != 68) {
        std::cerr << "Invalid handshake response\n";
        closesocket(sock);
        return false;
    }

    if (response[0] != 19 ||
        std::string((char*)&response[1], 19) != "BitTorrent protocol") {
        std::cerr << "Invalid peer protocol\n";
        closesocket(sock);
        return false;
        }

    std::cout << "Handshake successful with " << ip << ":" << port << "\n";
    closesocket(sock);
    return true;
}
