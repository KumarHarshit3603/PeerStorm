#pragma once
#include <cstdint>
#include <string>
#include <vector>

enum TorrentSourceType{
    TORRENT_FILE,
    MAGNET,
    UNKNOWN
};

class TorrentDataFile{
public:
    int64_t length;
    std::vector<std::string> path;
};

class TorrentMetadata{
public:
    std::string announce;
    std::vector<std::string> announce_list;
    std::string name;
    int64_t piece_length;
    std::vector<uint8_t> pieces;
    std::vector<TorrentDataFile> files;
    std::string download_directory;
    uint64_t total_size=0;
    size_t piece_count=0;
    int64_t Unix_timestamp=0;
    std::string creation_date;   // formatted local date
    std::string created_by;
    std::string comment;
    std::vector<uint8_t> info_hash = std::vector<uint8_t>(20);
};

TorrentSourceType IdentifySourceType(std::string &input);
TorrentMetadata ParseFile(std::string &path);
void ParseMagnet(std::string &input);
