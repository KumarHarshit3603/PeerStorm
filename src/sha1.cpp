#include "../include/sha1.h"
#include <cstring>

using std::uint32_t;

static uint32_t rol(uint32_t value, size_t bits) {
    return (value << bits) | (value >> (32 - bits));
}

std::vector<uint8_t> sha1(const std::string &data) {
    uint32_t h0=0x67452301, h1=0xEFCDAB89, h2=0x98BADCFE;
    uint32_t h3=0x10325476, h4=0xC3D2E1F0;

    std::vector<uint8_t> msg(data.begin(), data.end());
    size_t originalLen = msg.size() * 8;

    msg.push_back(0x80);
    while ((msg.size() * 8) % 512 != 448)
        msg.push_back(0x00);

    for (int i = 7; i >= 0; --i)
        msg.push_back(originalLen >> (i * 8));

    for (size_t i = 0; i < msg.size(); i += 64) {
        uint32_t w[80];
        for (int j = 0; j < 16; j++) {
            w[j] = (msg[i + j*4] << 24) |
                   (msg[i + j*4 + 1] << 16) |
                   (msg[i + j*4 + 2] << 8) |
                   (msg[i + j*4 + 3]);
        }
        for (int j = 16; j < 80; j++)
            w[j] = rol(w[j-3] ^ w[j-8] ^ w[j-14] ^ w[j-16], 1);

        uint32_t a=h0, b=h1, c=h2, d=h3, e=h4;

        for (int j = 0; j < 80; j++) {
            uint32_t f, k;
            if (j < 20) { f = (b & c) | (~b & d); k = 0x5A827999; }
            else if (j < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1; }
            else if (j < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
            else { f = b ^ c ^ d; k = 0xCA62C1D6; }

            uint32_t temp = rol(a,5) + f + e + k + w[j];
            e = d;
            d = c;
            c = rol(b,30);
            b = a;
            a = temp;
        }

        h0 += a; h1 += b; h2 += c; h3 += d; h4 += e;
    }

    std::vector<uint8_t> digest(20);
    uint32_t tmp[5] = {h0,h1,h2,h3,h4};
    for (int i=0; i<5; i++) {
        digest[i*4]     = (tmp[i] >> 24) & 0xff;
        digest[i*4 + 1] = (tmp[i] >> 16) & 0xff;
        digest[i*4 + 2] = (tmp[i] >> 8) & 0xff;
        digest[i*4 + 3] = tmp[i] & 0xff;
    }
    return digest;
}
