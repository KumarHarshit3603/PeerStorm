#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "include/peer_id.h"
#include "include/parser.h"
#include "include/bencode.h"
#include "include/sha1.h"
#include "include/tracker_http.h"
#include "include/magnet_parser.h"
using namespace std;

#include <vector>
#include <string>
#include <cstdint>

std::string infoHashToString(const std::vector<uint8_t>& infoHash) {
    return std::string(
        reinterpret_cast<const char*>(infoHash.data()),
        infoHash.size()
    );
}

void printTorrentMetadata(const TorrentMetadata& meta) {
    cout << "=== Torrent Information ===" << endl;
    cout << "Name: " << meta.name << endl;
    cout << "Announce: " << meta.announce << endl;

    if (!meta.announce_list.empty()) {
        cout << "Announce List:" << endl;
        for (auto &tracker : meta.announce_list)
            cout << "  - " << tracker << endl;
    }

    cout << "Created by: " << meta.created_by << endl;
    cout << "Unix timestamp: " << meta.Unix_timestamp << endl;
    cout << "Creation date: " << meta.creation_date << endl;
    cout << "Comment: " << meta.comment << endl;
    cout << "Piece length: " << meta.piece_length << endl;
    cout << "Total pieces: " << meta.piece_count << endl;
    cout << "Total size: " << meta.total_size << " bytes" << endl;

    if (!meta.files.empty()) {
        cout << "Files:" << endl;
        for (auto &f : meta.files) {
            cout << "  - Path: ";
            for (auto &p : f.path) cout << p << "/";
            cout << " | Size: " << f.length << " bytes" << endl;
        }
    }

    cout << "Info hash (SHA1): ";
    for (auto byte : meta.info_hash)
        printf("%02x", byte);
    cout << endl;
    cout<<infoHashToString(meta.info_hash)<<endl;
}
void printMagnetdata(const MagnetData& magdata ){
    cout<<"info hash hex"<<magdata.info_hash_hex<<endl;
    cout<<"trackers:"<<magdata.trackers.size()<<endl;
    cout<<"web seeds:"<<magdata.web_seeds.size()<<endl;
}
int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " add-torrent <torrent path or magnet link>" << endl;
        return 1;
    }

    string command = argv[1];
    string input = argv[2];

    if (command != "add-torrent") {
        cerr << "Unknown command: " << command << endl;
        return 1;
    }

    TorrentSourceType type = IdentifySourceType(input);
    std::string peerId = generatePeerId();
    if (type == TORRENT_FILE) {
        cout << "Loading torrent file: " << input << endl;
        TorrentMetadata meta = ParseFile(input);
        printTorrentMetadata(meta);
        cout<<peerId;


        std::vector<Peer> allPeers;
        set<std::string> seen; // to avoid duplicates

        for (const auto& tracker : meta.announce_list) {
            auto peers = announceHTTP(tracker, infoHashToString(meta.info_hash), peerId, meta.total_size);
            for (const auto& p : peers) {
                std::string key = p.ip + ":" + std::to_string(p.port);
                if (seen.find(key) == seen.end()) {
                    allPeers.push_back(p);
                    seen.insert(key);
                }
            }
        }

        std::cout << "Total peers collected: " << allPeers.size() << "\n";
        for (const auto& p : allPeers) {
            std::cout << p.ip << ":" << p.port << "\n";
        }



    } else if (type == MAGNET) {
        cout << "deciphering magnet link:"<<input << endl;
        MagnetData magdata = ParseMagnet(input);
        printMagnetdata(magdata);

    } else {
        cerr << "Invalid input" << endl;
    }
    return 0;
}
