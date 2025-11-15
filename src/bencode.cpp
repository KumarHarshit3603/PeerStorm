#include "../include/bencode.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

// -------- INTEGER --------
long long decodeInt(const std::string& data, size_t& pos) {
    if (pos >= data.size() || data[pos] != 'i')
        throw std::runtime_error("decodeInt: expected 'i' at position " + std::to_string(pos));
    pos++; // skip 'i'

    size_t end = data.find('e', pos);
    if (end == std::string::npos)
        throw std::runtime_error("Invalid bencode integer");

    long long val = std::stoll(data.substr(pos, end - pos));
    pos = end + 1;
    return val;
}

// -------- STRING --------
std::string decodeString(const std::string& data, size_t& pos) {
    // parse length
    size_t colon = data.find(':', pos);
    if (colon == std::string::npos)
        throw std::runtime_error("Invalid bencode string (no colon)");

    // length must be digits
    for (size_t i = pos; i < colon; ++i)
        if (!isdigit(static_cast<unsigned char>(data[i])))
            throw std::runtime_error("Invalid string length digits in bencode");

    size_t len = std::stoul(data.substr(pos, colon - pos));
    pos = colon + 1;

    if (pos + len > data.size())
        throw std::runtime_error("Invalid string length (out of range)");

    std::string s = data.substr(pos, len);
    pos += len;
    return s;
}

// -------- LIST --------
BList decodeList(const std::string& data, size_t& pos) {
    if (pos >= data.size() || data[pos] != 'l')
        throw std::runtime_error("decodeList: expected 'l' at position " + std::to_string(pos));
    pos++; // skip 'l'
    BList list;

    while (pos < data.size() && data[pos] != 'e') {
        list.push_back(decodeValue(data, pos));
    }
    if (pos >= data.size() || data[pos] != 'e')
        throw std::runtime_error("decodeList: unterminated list");
    pos++; // skip 'e'
    return list;
}

// -------- DICTIONARY --------
BDict decodeDict(const std::string& data, size_t& pos) {
    if (pos >= data.size() || data[pos] != 'd')
        throw std::runtime_error("decodeDict: expected 'd' at position " + std::to_string(pos));
    pos++; // skip 'd'
    BDict dict;

    while (pos < data.size() && data[pos] != 'e') {
        // Parse key manually (do not use decodeString which returns a BValue with offsets).
        size_t colon = data.find(':', pos);
        if (colon == std::string::npos)
            throw std::runtime_error("decodeDict: malformed key (no colon)");

        // ensure digits only for length
        for (size_t i = pos; i < colon; ++i)
            if (!isdigit(static_cast<unsigned char>(data[i])))
                throw std::runtime_error("decodeDict: invalid key length digits");

        size_t keylen = std::stoul(data.substr(pos, colon - pos));
        pos = colon + 1;

        if (pos + keylen > data.size())
            throw std::runtime_error("decodeDict: key length out of range");

        std::string key = data.substr(pos, keylen);
        pos += keylen;

        BValue val = decodeValue(data, pos);
        dict.emplace(std::move(key), std::move(val));
    }

    if (pos >= data.size() || data[pos] != 'e')
        throw std::runtime_error("decodeDict: unterminated dictionary");
    pos++; // skip 'e'
    return dict;
}

// -------- MASTER DECODER --------
BValue decodeValue(const std::string& data, size_t& pos) {
    if (pos >= data.size())
        throw std::runtime_error("decodeValue: out of range");

    size_t value_start = pos;
    char c = data[pos];

    if (c == 'i') {
        long long v = decodeInt(data, pos);
        BValue value(v);
        value.raw_start = value_start;
        value.raw_end = pos;
        return value;
    }
    if (std::isdigit(static_cast<unsigned char>(c))) {
        std::string s = decodeString(data, pos);
        BValue value(s);
        value.raw_start = value_start;
        value.raw_end = pos;
        return value;
    }
    if (c == 'l') {
        // mark list start at pos (points to 'l')
        size_t list_start = pos;
        BList lst = decodeList(data, pos);
        BValue value(lst);
        value.raw_start = list_start;
        value.raw_end = pos;
        return value;
    }
    if (c == 'd') {
        // mark dict start at pos (points to 'd')
        size_t dict_start = pos;
        BDict d = decodeDict(data, pos);
        BValue value(d);
        value.raw_start = dict_start;
        value.raw_end = pos;
        return value;
    }

    throw std::runtime_error("decodeValue: invalid bencode value at position " + std::to_string(pos));
}

string bencode_value(const BValue &v) {
    switch (v.type) {
    case BType::INT:
        return "i" + to_string(v.asInt()) + "e";

    case BType::STRING: {
            const string &s = v.asString();
            return to_string(s.size()) + ":" + s;
    }

    case BType::LIST: {
            string out = "l";
            for (const auto &item : v.asList()) {
                out += bencode_value(item);
            }
            out += "e";
            return out;
    }

    case BType::DICT: {
            string out = "d";
            // Bencode requires dictionary keys sorted lexicographically!
            vector<pair<string, BValue>> sorted;

            for (const auto &kv : v.asDict()) {
                sorted.push_back(kv);
            }

            sort(sorted.begin(), sorted.end(),
                 [](const auto &a, const auto &b) {
                     return a.first < b.first;
                 });

            for (const auto &kv : sorted) {
                const string &key = kv.first;
                const BValue &val = kv.second;

                out += to_string(key.size()) + ":" + key;
                out += bencode_value(val);
            }

            out += "e";
            return out;
    }

    default:
        throw runtime_error("bencode_value: invalid type");
    }
}

// skipBencodeValue: advance pos over ONE bencoded value (string, int, list, dict)
void skipBencodeValue(const std::string &data, size_t &pos) {
    if (pos >= data.size()) throw std::runtime_error("skipBencodeValue: out of range");
    char c = data[pos];

    if (c == 'i') {
        pos++;
        size_t end = data.find('e', pos);
        if (end == std::string::npos) throw std::runtime_error("malformed integer");
        pos = end + 1;
        return;
    }
    if (std::isdigit(static_cast<unsigned char>(c))) {
        size_t colon = data.find(':', pos);
        if (colon == std::string::npos) throw std::runtime_error("malformed string");
        // validate digits
        for (size_t i = pos; i < colon; ++i)
            if (!isdigit(static_cast<unsigned char>(data[i])))
                throw std::runtime_error("malformed string length digits");
        size_t len = std::stoull(data.substr(pos, colon - pos));
        pos = colon + 1;
        if (pos + len > data.size()) throw std::runtime_error("string length out of range");
        pos += len;
        return;
    }
    if (c == 'l') {
        pos++;
        while (pos < data.size() && data[pos] != 'e')
            skipBencodeValue(data, pos);
        if (pos >= data.size() || data[pos] != 'e')
            throw std::runtime_error("unterminated list");
        pos++;
        return;
    }
    if (c == 'd') {
        pos++;
        while (pos < data.size() && data[pos] != 'e') {
            size_t colon = data.find(':', pos);
            if (colon == std::string::npos) throw std::runtime_error("malformed dict key");
            // validate digits
            for (size_t i = pos; i < colon; ++i)
                if (!isdigit(static_cast<unsigned char>(data[i])))
                    throw std::runtime_error("malformed dict key length digits");
            size_t keylen = std::stoull(data.substr(pos, colon - pos));
            pos = colon + 1;
            if (pos + keylen > data.size()) throw std::runtime_error("dict key out of range");
            pos += keylen;
            skipBencodeValue(data, pos);
        }
        if (pos >= data.size() || data[pos] != 'e')
            throw std::runtime_error("unterminated dict");
        pos++;
        return;
    }
    throw std::runtime_error("skipBencodeValue: unknown token at " + std::to_string(pos));
}

std::pair<size_t, size_t> findInfoValueRange(const std::string& data) {
    size_t pos = 0;
    if (data.empty() || data[pos] != 'd') throw std::runtime_error("Root is not a dictionary");
    pos++; // skip root 'd'

    while (pos < data.size() && data[pos] != 'e') {
        size_t colon = data.find(':', pos);
        if (colon == std::string::npos) throw std::runtime_error("Invalid key (no colon)");
        // validate digits for key length
        for (size_t i = pos; i < colon; ++i)
            if (!isdigit(static_cast<unsigned char>(data[i])))
                throw std::runtime_error("Invalid key length digits in root dictionary");
        size_t key_len = std::stoull(data.substr(pos, colon - pos));
        pos = colon + 1;
        if (pos + key_len > data.size()) throw std::runtime_error("Invalid key length (out of range)");
        std::string key = data.substr(pos, key_len);
        pos += key_len;

        if (key == "info") {
            size_t start = pos;
            size_t end = pos;
            // skip the value into 'end' so it points just after the end of the value
            skipBencodeValue(data, end);
            return {start, end};
        } else {
            skipBencodeValue(data, pos);
        }
    }
    throw std::runtime_error("Key 'info' not found in torrent file");
}
