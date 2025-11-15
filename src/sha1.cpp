#include "../include/sha1.h"
#include <cstring>
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <iomanip>

using std::uint32_t;
using std::uint8_t;
using std::vector;

static uint32_t rol(uint32_t value, unsigned int bits) {
    return (value << bits) | (value >> (32 - bits));
}

// Internal worker: compute sha1 from bytes in vector<uint8_t>
static vector<uint8_t> sha1_from_bytes(const vector<uint8_t> &msg_in) {
    // Initialize hash values
    uint32_t h0 = 0x67452301;
    uint32_t h1 = 0xEFCDAB89;
    uint32_t h2 = 0x98BADCFE;
    uint32_t h3 = 0x10325476;
    uint32_t h4 = 0xC3D2E1F0;

    // Make a working copy and pad
    vector<uint8_t> msg = msg_in;
    uint64_t originalBitLen = static_cast<uint64_t>(msg.size()) * 8ULL;

    // append 0x80
    msg.push_back(0x80);
    // append zeros until length in bytes ≡ 56 (mod 64)
    while ((msg.size() % 64) != 56) msg.push_back(0x00);

    // append 64-bit big-endian length
    for (int i = 7; i >= 0; --i) {
        msg.push_back(static_cast<uint8_t>((originalBitLen >> (i * 8)) & 0xFF));
    }

    // process in 512-bit (64-byte) chunks
    for (size_t chunk = 0; chunk < msg.size(); chunk += 64) {
        uint32_t w[80];
        // build message schedule (note explicit cast to uint32_t before shift)
        for (int j = 0; j < 16; ++j) {
            size_t idx = chunk + j * 4;
            w[j] = (static_cast<uint32_t>(msg[idx]) << 24)
                 | (static_cast<uint32_t>(msg[idx + 1]) << 16)
                 | (static_cast<uint32_t>(msg[idx + 2]) << 8)
                 | (static_cast<uint32_t>(msg[idx + 3]));
        }
        for (int j = 16; j < 80; ++j) {
            w[j] = rol(w[j-3] ^ w[j-8] ^ w[j-14] ^ w[j-16], 1);
        }

        uint32_t a = h0;
        uint32_t b = h1;
        uint32_t c = h2;
        uint32_t d = h3;
        uint32_t e = h4;

        for (int j = 0; j < 80; ++j) {
            uint32_t f, k;
            if (j < 20) {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999;
            } else if (j < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (j < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }

            uint32_t temp = rol(a, 5) + f + e + k + w[j];
            e = d;
            d = c;
            c = rol(b, 30);
            b = a;
            a = temp;
        }

        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }

    vector<uint8_t> digest(20);
    uint32_t tmp[5] = {h0, h1, h2, h3, h4};
    for (int i = 0; i < 5; ++i) {
        digest[i*4 + 0] = static_cast<uint8_t>((tmp[i] >> 24) & 0xFF);
        digest[i*4 + 1] = static_cast<uint8_t>((tmp[i] >> 16) & 0xFF);
        digest[i*4 + 2] = static_cast<uint8_t>((tmp[i] >> 8) & 0xFF);
        digest[i*4 + 3] = static_cast<uint8_t>((tmp[i]) & 0xFF);
    }
    return digest;
}

// Public API: old sha1(std::string) — keep for backwards compatibility
std::vector<uint8_t> sha1(const std::string &data) {
    vector<uint8_t> vec(data.begin(), data.end());
    return sha1_from_bytes(vec);
}

// NEW API: hash raw bytes directly
std::vector<uint8_t> sha1_bytes(const std::vector<uint8_t> &data) {
    return sha1_from_bytes(data);
}
