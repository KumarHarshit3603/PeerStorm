#pragma once
#include <cstdint>
#include <iomanip>
#include <string>
#include <vector>
#include <map>


class  MagnetData {
public:
    std::vector<uint8_t> info_hash_bytes;   // raw 20 bytes
    std::string info_hash_hex;              // hex representation

    std::string display_name; // dn
    std::vector<std::string> trackers;       // tr
    std::vector<std::string> web_seeds;      // ws
    long long file_size;      // xl

    // store all other params as key -> list of values
    std::map<std::string, std::vector<std::string>> extra_params;
};

// ================= Helper Functions ====================

// percent-decoding
static std::string urlDecode(const std::string &s) {
    std::string out;
    out.reserve(s.size());
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size()) {
            int hi = std::stoi(s.substr(i + 1, 1), nullptr, 16);
            int lo = std::stoi(s.substr(i + 2, 1), nullptr, 16);
            out.push_back((char)((hi << 4) | lo));
            i += 2;
        } else if (s[i] == '+') {
            out.push_back(' ');
        } else {
            out.push_back(s[i]);
        }
    }
    return out;
}

// convert hex to bytes
static std::vector<uint8_t> hexBytes(const std::string &hex) {
    if (hex.size() % 2 != 0) throw std::runtime_error("Invalid hex length");
    std::vector<uint8_t> out(hex.size() / 2);
    for (size_t i = 0; i < out.size(); i++) {
        out[i] = (uint8_t)std::stoi(hex.substr(2*i, 2), nullptr, 16);
    }
    return out;
}

// convert bytes -> lowercase hex
static std::string toHex(const std::vector<uint8_t> &bytes) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (auto b : bytes) oss << std::setw(2) << (int)b;
    return oss.str();
}

// RFC4648 base32 decode
static std::vector<uint8_t> base32Decode(const std::string &input) {
    static const char *alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    int lookup[256];
    std::fill(std::begin(lookup), std::end(lookup), -1);

    for (int i = 0; i < 32; i++) {
        lookup[(unsigned char)alphabet[i]] = i;
        lookup[(unsigned char)std::tolower(alphabet[i])] = i;
    }

    std::vector<uint8_t> output;
    int buffer = 0, bits = 0;

    for (char c : input) {
        if (lookup[(unsigned char)c] == -1) continue;
        buffer = (buffer << 5) | lookup[(unsigned char)c];
        bits += 5;

        if (bits >= 8) {
            bits -= 8;
            output.push_back((buffer >> bits) & 0xFF);
        }
    }
    return output;
}

// Trim
static std::string trim(const std::string &s){
    size_t a = 0, b = s.size();
    while (a < b && isspace(s[a])) a++;
    while (b > a && isspace(s[b - 1])) b--;
    return s.substr(a, b - a);
}

