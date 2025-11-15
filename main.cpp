#include <iostream>
#include <string>
#include <vector>
#include "include/parser.h"
#include "include/bencode.h"
#include "include/sha1.h"
using namespace std;

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

    if (type == TORRENT_FILE) {
        cout << "Loading torrent file: " << input << endl;
        TorrentMetadata meta = ParseFile(input);
        printTorrentMetadata(meta);
    } else if (type == MAGNET) {
        cout << "Magnet link parsing not implemented yet" << endl;
    } else {
        cerr << "Invalid input" << endl;
    }
    return 0;
}
