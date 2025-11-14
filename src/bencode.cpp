#include "../include/bencode.h"

#include <algorithm>
#include <cctype>
using namespace std;

// -------- INTEGER --------
long long decodeInt(const std::string& data, size_t& pos) {
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
    size_t colon = data.find(':', pos);
    if (colon == std::string::npos)
        throw std::runtime_error("Invalid bencode string");

    size_t len = std::stoul(data.substr(pos, colon - pos));
    pos = colon + 1;

    if (pos + len > data.size())
        throw std::runtime_error("Invalid string length");

    std::string s = data.substr(pos, len);
    pos += len;
    return s;
}

// -------- LIST --------
BList decodeList(const std::string& data, size_t& pos) {
    pos++; // skip 'l'
    BList list;

    while (data[pos] != 'e') {
        list.push_back(decodeValue(data, pos));
    }
    pos++; // skip 'e'
    return list;
}

// -------- DICTIONARY --------
BDict decodeDict(const std::string& data, size_t& pos) {
    pos++; // skip 'd'
    BDict dict;

    while (data[pos] != 'e') {
        std::string key = decodeString(data, pos);
        BValue val = decodeValue(data, pos);
        dict[key] = val;
    }

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
        BList lst = decodeList(data, pos);
        BValue value(lst);
        value.raw_start = value_start;
        value.raw_end = pos;
        return value;
    }
    if (c == 'd') {
        BDict d = decodeDict(data, pos);
        BValue value(d);
        value.raw_start = value_start;
        value.raw_end = pos;
        return value;
    }

    throw std::runtime_error("Invalid bencode value at position " + std::to_string(pos));
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

// ---------------------
// skipBencodeValue()
// ---------------------
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
        size_t len = std::stoull(data.substr(pos, colon - pos));
        pos = colon + 1;
        pos += len;
        return;
    }
    if (c == 'l') {
        pos++;
        while (data[pos] != 'e') skipBencodeValue(data, pos);
        pos++;
        return;
    }
    if (c == 'd') {
        pos++;
        while (data[pos] != 'e') {
            size_t colon = data.find(':', pos);
            size_t keylen = std::stoull(data.substr(pos, colon - pos));
            pos = colon + 1;
            pos += keylen;
            skipBencodeValue(data, pos);
        }
        pos++;
        return;
    }
}

// ---------------------
// findInfoValueRange()
// ---------------------
std::pair<size_t, size_t> findInfoValueRange(const std::string& data) {
    size_t pos = 0;
    if (data[pos] != 'd') throw std::runtime_error("Root is not a dictionary");
    pos++;

    while (pos < data.size() && data[pos] != 'e') {
        size_t colon = data.find(':', pos);
        size_t key_len = std::stoull(data.substr(pos, colon - pos));
        pos = colon + 1;
        std::string key = data.substr(pos, key_len);
        pos += key_len;

        if (key == "info") {
            size_t start = pos;
            size_t end = pos;
            skipBencodeValue(data, end);
            return { start, end };
        } else {
            skipBencodeValue(data, pos);
        }
    }
    throw std::runtime_error("Key 'info' not found");
}
