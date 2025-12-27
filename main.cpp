#include <iostream>
#include <set>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

#include <winsock2.h>
#include <windows.h>

#include "include/peer_id.h"
#include "include/parser.h"
#include "include/bencode.h"
#include "include/sha1.h"
#include "include/tracker_http.h"
#include "include/magnet_parser.h"
#include "include/tracker_udp.h"
#include "include/peer_connection.h"
#include "include/peer_handshake.h"
#include "include/piece_manager.h"
#include "include/peer_state_machine.h"

using namespace std;

constexpr size_t MAX_ACTIVE_PEERS = 40;
std::mutex coutMutex;

/* Convert 20-byte info-hash to raw string (binary safe) */
std::string infoHashToString(const std::vector<uint8_t>& infoHash) {
    return std::string(
        reinterpret_cast<const char*>(infoHash.data()),
        infoHash.size()
    );
}

void printTorrentMetadata(const TorrentMetadata& meta) {
    cout << "=== Torrent Information ===\n";
    cout << "Name: " << meta.name << "\n";
    cout << "Announce: " << meta.announce << "\n";

    if (!meta.announce_list.empty()) {
        cout << "Announce List:\n";
        for (const auto& t : meta.announce_list)
            cout << "  - " << t << "\n";
    }

    cout << "Piece length: " << meta.piece_length << "\n";
    cout << "Total pieces: " << meta.piece_count << "\n";
    cout << "Total size: " << meta.total_size << " bytes\n";

    cout << "Info hash (hex): ";
    for (uint8_t b : meta.info_hash)
        printf("%02x", b);
    cout << "\n";
}

void printMagnetData(const MagnetData& mag) {
    cout << "Info hash (hex): " << mag.info_hash_hex << "\n";
    cout << "Trackers: " << mag.trackers.size() << "\n";
    cout << "Web seeds: " << mag.web_seeds.size() << "\n";
}

int main(int argc, char* argv[]) {

    /* ---------------- Winsock Init ---------------- */
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        cerr << "WSAStartup failed\n";
        return 1;
    }

    if (argc < 3) {
        cerr << "Usage: " << argv[0]
             << " add-torrent <torrent file | magnet link>\n";
        WSACleanup();
        return 1;
    }

    string command = argv[1];
    string input   = argv[2];

    if (command != "add-torrent") {
        cerr << "Unknown command\n";
        WSACleanup();
        return 1;
    }

    TorrentSourceType type = IdentifySourceType(input);
    string peerId = generatePeerId();

    /* =====================================================
       =============== TORRENT FILE =========================
       ===================================================== */
    if (type == TORRENT_FILE) {

        TorrentMetadata meta = ParseFile(input);
        printTorrentMetadata(meta);

        vector<Peer> allPeers;
        set<string> seen;

        for (const auto& tracker : meta.announce_list) {
            vector<Peer> peers;

            if (tracker.rfind("udp://", 0) == 0) {
                peers = announceUDP(
                    tracker,
                    infoHashToString(meta.info_hash),
                    peerId,
                    meta.total_size
                );
            }
            else if (tracker.rfind("http://", 0) == 0) {
                peers = announceHTTP(
                    tracker,
                    infoHashToString(meta.info_hash),
                    peerId,
                    meta.total_size
                );
            }

            for (const auto& p : peers) {
                string key = p.ip + ":" + to_string(p.port);
                if (seen.insert(key).second)
                    allPeers.push_back(p);
            }
        }

        {
            lock_guard<mutex> lock(coutMutex);
            cout << "Total peers collected: "
                 << allPeers.size() << "\n";
        }

        /* ---------------- Piece Manager ---------------- */
        PieceManager pieceManager(
    static_cast<int>(meta.piece_count),
    static_cast<int>(meta.piece_length)
);


        /* ---------------- Peer Threads ---------------- */
        vector<thread> peerThreads;
        atomic<int> activePeers{0};

        auto handshake = buildHandshake(
            infoHashToString(meta.info_hash),
            peerId
        );

        for (const auto& p : allPeers) {
            peerThreads.emplace_back([&, p]() {

                int prev = activePeers.fetch_add(1);
                if (prev >= MAX_ACTIVE_PEERS) {
                    activePeers--;
                    return;
                }

                SOCKET sock = connectToPeer(
                    p.ip,
                    p.port,
                    handshake
                );

                if (sock == INVALID_SOCKET) {
                    {
                        lock_guard<mutex> lock(coutMutex);
                        cout << "Connection failed: "
                             << p.ip << ":" << p.port << "\n";
                    }
                    activePeers--;
                    return;
                }

                {
                    lock_guard<mutex> lock(coutMutex);
                    cout << "[+] Connected: "
                         << p.ip << ":" << p.port << "\n";
                }

                PeerStateMachine psm(
                    p.ip,
                    p.port,
                    sock,
                    &pieceManager
                );

                psm.start();

                closesocket(sock);
                activePeers--;
            });
        }

        for (auto& t : peerThreads) {
            if (t.joinable())
                t.join();
        }
    }

    /* =====================================================
       ================= MAGNET LINK ========================
       ===================================================== */
    else if (type == MAGNET) {
        MagnetData mag = ParseMagnet(input);
        printMagnetData(mag);
    }
    else {
        cerr << "Invalid input\n";
    }

    WSACleanup();
    return 0;
}
