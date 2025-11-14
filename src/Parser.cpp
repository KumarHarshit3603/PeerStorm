#include <fstream>
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include "../include/parser.h"
#include "../include/bencode.h"
#include "../include/sha1.h"

using namespace std;

bool fileExists(const std::string& path) {
    std::ifstream f(path);
    return f.good();
}

TorrentSourceType IdentifySourceType(string &input) {
    if (input.rfind("magnet:", 0) == 0)
        return MAGNET;
    else if (fileExists(input))
        return TORRENT_FILE;
    else
        return UNKNOWN;
}

std::string unixTimestampToLocalTime(long long timestamp) {
    using namespace std::chrono;
    auto time_point = system_clock::time_point(seconds(timestamp));
    std::time_t time = system_clock::to_time_t(time_point);
    std::tm* local_tm = std::localtime(&time);

    std::ostringstream oss;
    oss << std::put_time(local_tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

TorrentMetadata ParseFile(string &path) {
    TorrentMetadata meta;

    // --- Read file ---
    ifstream file(path, ios::binary);
    if (!file)
        throw runtime_error("Cannot open .torrent file");

    string data((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());

    size_t pos = 0;
    BValue root = decodeValue(data, pos);
    const BDict &dict = root.asDict();

    // --- Basic fields ---
    if (dict.count("announce"))
        meta.announce = dict.at("announce").asString();

    if (dict.count("announce-list")) {
        for (auto &tier : dict.at("announce-list").asList())
            for (auto &v : tier.asList())
                meta.announce_list.push_back(v.asString());
    }

    if (dict.count("creation date")) {
        meta.Unix_timestamp = dict.at("creation date").asInt();
        meta.creation_date = unixTimestampToLocalTime(meta.Unix_timestamp);
    }

    if (dict.count("created by"))
        meta.created_by = dict.at("created by").asString();

    if (dict.count("comment"))
        meta.comment = dict.at("comment").asString();

    // --- INFO DICTIONARY ---
    const BValue &info_bvalue = dict.at("info");
    const BDict &info = info_bvalue.asDict();

    meta.name = info.at("name").asString();
    meta.piece_length = info.at("piece length").asInt();

    string pieces_str = info.at("pieces").asString();
    meta.pieces.assign(pieces_str.begin(), pieces_str.end());
    meta.piece_count = meta.pieces.size() / 20;

    // MULTI-FILE MODE
    if (info.count("files")) {
        for (auto &item : info.at("files").asList()) {
            const BDict &fd = item.asDict();
            TorrentDataFile f;
            f.length = fd.at("length").asInt();
            for (auto &p : fd.at("path").asList())
                f.path.push_back(p.asString());
            meta.total_size += f.length;
            meta.files.push_back(f);
        }
    } else {
        TorrentDataFile f;
        f.length = info.at("length").asInt();
        f.path.push_back(meta.name);
        meta.total_size = f.length;
        meta.files.push_back(f);
    }

    // --- CORRECT INFO-HASH ---
    size_t info_start = info_bvalue.raw_start;
    size_t info_end   = info_bvalue.raw_end;

    std::string info_raw = data.substr(info_start, info_end - info_start);
    meta.info_hash = sha1(info_raw);

    return meta;
}

void ParseMagnet(string &input) {
    // TODO
}
