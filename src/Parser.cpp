// src/Parser.cpp
#include <fstream>
#include <iostream>
#include <string>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <vector>
#include <stdexcept>

#include "../include/parser.h"
#include "../include/bencode.h"
#include "../include/sha1.h"
#include "../include/magnet_parser.h"

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
    if (!local_tm) return "Invalid timestamp";
    std::ostringstream oss;
    oss << std::put_time(local_tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

// ------------------------------
// Byte-oriented bencode walker (mirrors your working BencodeParser logic)
// It only finds the raw start/end of the bencoded "info" value.
// ------------------------------
static bool parseValueBytes(const std::vector<unsigned char>& data, size_t &pos);

static bool parseStringBytes(const std::vector<unsigned char>& data, size_t &pos) {
    size_t n = data.size();
    if (pos >= n || !('0' <= data[pos] && data[pos] <= '9')) return false;
    size_t len = 0;
    while (pos < n && data[pos] >= '0' && data[pos] <= '9') {
        len = len * 10 + (data[pos] - '0');
        pos++;
    }
    if (pos >= n || data[pos] != ':') return false;
    pos++; // skip ':'
    if (pos + len > n) return false;
    pos += len;
    return true;
}

static bool parseIntegerBytes(const std::vector<unsigned char>& data, size_t &pos) {
    size_t n = data.size();
    if (pos >= n || data[pos] != 'i') return false;
    pos++; // skip 'i'
    while (pos < n && data[pos] != 'e') pos++;
    if (pos >= n || data[pos] != 'e') return false;
    pos++; // skip 'e'
    return true;
}

static bool parseListBytes(const std::vector<unsigned char>& data, size_t &pos) {
    size_t n = data.size();
    if (pos >= n || data[pos] != 'l') return false;
    pos++; // skip 'l'
    while (pos < n && data[pos] != 'e') {
        if (!parseValueBytes(data, pos)) return false;
    }
    if (pos >= n || data[pos] != 'e') return false;
    pos++; // skip 'e'
    return true;
}

static bool parseDictBytes(const std::vector<unsigned char>& data, size_t &pos) {
    size_t n = data.size();
    if (pos >= n || data[pos] != 'd') return false;
    pos++; // skip 'd'
    while (pos < n && data[pos] != 'e') {
        // parse key (must be a string length:...)
        if (!parseStringBytes(data, pos)) return false;
        // parse value
        if (!parseValueBytes(data, pos)) return false;
    }
    if (pos >= n || data[pos] != 'e') return false;
    pos++; // skip 'e'
    return true;
}

static bool parseValueBytes(const std::vector<unsigned char>& data, size_t &pos) {
    if (pos >= data.size()) return false;
    unsigned char t = data[pos];
    if (t >= '0' && t <= '9') return parseStringBytes(data, pos);
    if (t == 'i') return parseIntegerBytes(data, pos);
    if (t == 'l') return parseListBytes(data, pos);
    if (t == 'd') return parseDictBytes(data, pos);
    if (t == 'e') { pos++; return true; } // should rarely be used here
    return false;
}

// This function mirrors your working parser's findInfoDictionary logic.
// It returns true and sets [out_start, out_end) to the raw bencoded bytes of the value for "info".
static bool findInfoRangeFromBytes(const std::vector<unsigned char>& data, size_t &out_start, size_t &out_end) {
    out_start = out_end = static_cast<size_t>(-1);
    size_t n = data.size();
    if (n == 0) return false;
    if (data[0] != 'd') return false;

    size_t pos = 0;
    // Expect root dict
    if (data[pos++] != 'd') return false;

    // iterate root dict keys
    while (pos < n && data[pos] != 'e') {
        // key must be a string: <len>:<key>
        if (pos >= n || data[pos] < '0' || data[pos] > '9') return false;
        size_t key_len = 0;
        while (pos < n && data[pos] >= '0' && data[pos] <= '9') {
            key_len = key_len * 10 + (data[pos] - '0');
            pos++;
        }
        if (pos >= n || data[pos] != ':') return false;
        pos++; // skip ':'
        if (pos + key_len > n) return false;
        std::string key;
        key.reserve(key_len);
        for (size_t i = 0; i < key_len; ++i) {
            key.push_back(static_cast<char>(data[pos++]));
        }

        if (key == "info") {
            // start of info value is current pos
            size_t start_pos = pos;
            if (!parseValueBytes(data, pos)) return false;
            size_t end_pos = pos;
            out_start = start_pos;
            out_end = end_pos;
            return true;
        } else {
            // skip the value for this key
            if (!parseValueBytes(data, pos)) return false;
        }
    }

    return false;
}

// ------------------------------
// ParseFile implementation
// ------------------------------
TorrentMetadata ParseFile(string &path) {
    TorrentMetadata meta;

    // --- Read file into raw vector of bytes ---
    ifstream file(path, ios::binary);
    if (!file) throw runtime_error("Cannot open .torrent file");

    // read into vector<unsigned char>
    std::vector<unsigned char> raw;
    file.seekg(0, ios::end);
    std::streampos fsize = file.tellg();
    if (fsize <= 0) throw runtime_error("Empty or invalid file size");
    raw.resize(static_cast<size_t>(fsize));
    file.seekg(0, ios::beg);
    if (!file.read(reinterpret_cast<char*>(raw.data()), static_cast<std::streamsize>(raw.size())))
        throw runtime_error("Error reading file");

    // --- Find raw info slice using byte-walker that mirrors your working code ---
    size_t info_start = static_cast<size_t>(-1);
    size_t info_end = static_cast<size_t>(-1);
    if (!findInfoRangeFromBytes(raw, info_start, info_end)) {
        throw runtime_error("Could not locate 'info' dictionary in torrent (byte-scan failed)");
    }
    if (info_start >= info_end || info_end > raw.size()) {
        throw runtime_error("Invalid info byte range found");
    }

    // Extract raw bytes slice for hashing
    // Use a vector<uint8_t> to preserve raw bytes exactly (no std::string conversion)
    std::vector<uint8_t> info_bytes;
    info_bytes.reserve(info_end - info_start);
    info_bytes.insert(info_bytes.end(), raw.begin() + info_start, raw.begin() + info_end);

    // Compute SHA1 over the raw byte vector using sha1_bytes (added below)
    meta.info_hash = sha1_bytes(info_bytes);


    // --- Now decode the full file into BValue for the rest of metadata ---
    // convert raw to std::string for existing decoder functions
    std::string data_str;
    data_str.assign(reinterpret_cast<const char*>(raw.data()), raw.size());

    size_t pos = 0;
    BValue root = decodeValue(data_str, pos);
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

    // --- INFO DICTIONARY (parsed struct) ---
    const BValue &info_bvalue = dict.at("info"); // parsed object
    const BDict &info = info_bvalue.asDict();

    // fill metadata fields from parsed info
    if (info.count("name")) meta.name = info.at("name").asString();
    if (info.count("piece length")) meta.piece_length = info.at("piece length").asInt();

    // Pieces
    if (info.count("pieces")) {
        string pieces_str = info.at("pieces").asString();
        meta.pieces.assign(pieces_str.begin(), pieces_str.end());
        meta.piece_count = meta.pieces.size() / 20;
    }

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
        // SINGLE-FILE
        if (info.count("length")) {
            TorrentDataFile f;
            f.length = info.at("length").asInt();
            f.path.push_back(meta.name);
            meta.total_size = f.length;
            meta.files.push_back(f);
        }
    }

    return meta;
}

MagnetData ParseMagnet(string &input) {
 if (input.rfind("magnet:?", 0) != 0)
        throw std::runtime_error("Not a magnet link");

    std::string query = input.substr(8); // after "magnet:?"

    // split params by '&'
    std::vector<std::string> parts;
    std::stringstream ss(query);
    std::string part;

    while (std::getline(ss, part, '&'))
        parts.push_back(part);

    // Map of key -> list of values
    std::map<std::string, std::vector<std::string>> params;

    for (auto &p : parts) {
        auto eq = p.find('=');
        if (eq == std::string::npos)
            continue;

        std::string key = urlDecode(p.substr(0, eq));
        std::string value = urlDecode(p.substr(eq + 1));
        params[key].push_back(value);
    }

    MagnetData data;

    // ============ Extract xt (info-hash) ==============
    if (!params.count("xt"))
        throw std::runtime_error("Missing xt param");

    std::string xt = params["xt"].front();

    const std::string prefix = "urn:btih:";
    if (xt.rfind(prefix, 0) != 0)
        throw std::runtime_error("xt must start with urn:btih:");

    std::string hash = xt.substr(prefix.size());

    // hex (40 chars)
    if (hash.size() == 40) {
        data.info_hash_bytes = hexBytes(hash);
    }
    // base32 (32 chars)
    else if (hash.size() == 32) {
        data.info_hash_bytes = base32Decode(hash);
        if (data.info_hash_bytes.size() != 20)
            throw std::runtime_error("Base32 decode failure");
    } else {
        throw std::runtime_error("Invalid info-hash format");
    }

    data.info_hash_hex = toHex(data.info_hash_bytes);

    // ============ Extract dn ==============
    if (params.count("dn"))
        data.display_name = params["dn"].front();

    // ============ Extract trackers ==============
    if (params.count("tr"))
        data.trackers = params["tr"];

    // ============ Extract web seeds (ws) ==============
    if (params.count("ws"))
        data.web_seeds = params["ws"];

    // ============ Extract file size (xl) ==============
    if (params.count("xl"))
        data.file_size = std::stoll(params["xl"].front());

    // ============ Store unknown params ==============
    for (auto &kv : params) {
        if (kv.first == "xt" || kv.first == "dn" || kv.first == "tr" ||
            kv.first == "ws" || kv.first == "xl")
            continue;

        data.extra_params[kv.first] = kv.second;
    }

    return data;

}
