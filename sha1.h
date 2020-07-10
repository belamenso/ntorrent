//
// Created by julian on 09.07.2020.
// https://github.com/983/SHA1
//

#ifndef NTORRENT_SHA1_H
#define NTORRENT_SHA1_H

#include <cstdint>
#include <cstring>
#include <string>
#include <cassert>

class sha1 {
private:

    void add_byte_dont_count_bits(uint8_t x) {
        buf[bi++] = x;
        if (bi >= sizeof(buf)) {
            assert(bi == sizeof(buf));
            bi = 0;
            process_block(buf);
        }
    }

    static uint32_t rol32(uint32_t x, uint32_t n) {
        return (x << n) | (x >> (32 - n));
    }

    static uint32_t make_word(const uint8_t *p) {
        return
            ((uint32_t)p[0] << 3*8u) |
            ((uint32_t)p[1] << 2*8u) |
            ((uint32_t)p[2] << 1*8u) |
            ((uint32_t)p[3] << 0*8u);
    }

    void process_block(const uint8_t *ptr) {
        const uint32_t c0 = 0x5a827999;
        const uint32_t c1 = 0x6ed9eba1;
        const uint32_t c2 = 0x8f1bbcdc;
        const uint32_t c3 = 0xca62c1d6;

        uint32_t a = state[0];
        uint32_t b = state[1];
        uint32_t c = state[2];
        uint32_t d = state[3];
        uint32_t e = state[4];

        uint32_t w[16];

        for (int i = 0; i < 16; i++) w[i] = make_word(ptr + i*4);

#define SHA1_LOAD(i) w[i&15u] = rol32(w[(i+13)&15u] ^ w[(i+8)&15u] ^ w[(i+2)&15u] ^ w[i&15u], 1);
#define SHA1_ROUND_0(v,u,x,y,z,i)              z += ((u & (x ^ y)) ^ y) + w[i&15u] + c0 + rol32(v, 5u); u = rol32(u, 30u);
#define SHA1_ROUND_1(v,u,x,y,z,i) SHA1_LOAD(i) z += ((u & (x ^ y)) ^ y) + w[i&15u] + c0 + rol32(v, 5u); u = rol32(u, 30u);
#define SHA1_ROUND_2(v,u,x,y,z,i) SHA1_LOAD(i) z += (u ^ x ^ y) + w[i&15u] + c1 + rol32(v, 5u); u = rol32(u, 30u);
#define SHA1_ROUND_3(v,u,x,y,z,i) SHA1_LOAD(i) z += (((u | x) & y) | (u & x)) + w[i&15u] + c2 + rol32(v, 5); u = rol32(u, 30u);
#define SHA1_ROUND_4(v,u,x,y,z,i) SHA1_LOAD(i) z += (u ^ x ^ y) + w[i&15u] + c3 + rol32(v, 5u); u = rol32(u, 30u);

        SHA1_ROUND_0(a, b, c, d, e,  0u);
        SHA1_ROUND_0(e, a, b, c, d,  1u);
        SHA1_ROUND_0(d, e, a, b, c,  2u);
        SHA1_ROUND_0(c, d, e, a, b,  3u);
        SHA1_ROUND_0(b, c, d, e, a,  4u);
        SHA1_ROUND_0(a, b, c, d, e,  5u);
        SHA1_ROUND_0(e, a, b, c, d,  6u);
        SHA1_ROUND_0(d, e, a, b, c,  7u)
        SHA1_ROUND_0(c, d, e, a, b,  8u);
        SHA1_ROUND_0(b, c, d, e, a,  9u);
        SHA1_ROUND_0(a, b, c, d, e, 10u);
        SHA1_ROUND_0(e, a, b, c, d, 11u);
        SHA1_ROUND_0(d, e, a, b, c, 12u);
        SHA1_ROUND_0(c, d, e, a, b, 13u);
        SHA1_ROUND_0(b, c, d, e, a, 14u);
        SHA1_ROUND_0(a, b, c, d, e, 15u);
        SHA1_ROUND_1(e, a, b, c, d, 16u);
        SHA1_ROUND_1(d, e, a, b, c, 17u);
        SHA1_ROUND_1(c, d, e, a, b, 18u);
        SHA1_ROUND_1(b, c, d, e, a, 19u);
        SHA1_ROUND_2(a, b, c, d, e, 20u);
        SHA1_ROUND_2(e, a, b, c, d, 21u);
        SHA1_ROUND_2(d, e, a, b, c, 22u);
        SHA1_ROUND_2(c, d, e, a, b, 23u);
        SHA1_ROUND_2(b, c, d, e, a, 24u);
        SHA1_ROUND_2(a, b, c, d, e, 25u);
        SHA1_ROUND_2(e, a, b, c, d, 26u);
        SHA1_ROUND_2(d, e, a, b, c, 27u);
        SHA1_ROUND_2(c, d, e, a, b, 28u);
        SHA1_ROUND_2(b, c, d, e, a, 29u);
        SHA1_ROUND_2(a, b, c, d, e, 30u);
        SHA1_ROUND_2(e, a, b, c, d, 31u);
        SHA1_ROUND_2(d, e, a, b, c, 32u);
        SHA1_ROUND_2(c, d, e, a, b, 33u);
        SHA1_ROUND_2(b, c, d, e, a, 34u);
        SHA1_ROUND_2(a, b, c, d, e, 35u);
        SHA1_ROUND_2(e, a, b, c, d, 36u);
        SHA1_ROUND_2(d, e, a, b, c, 37u);
        SHA1_ROUND_2(c, d, e, a, b, 38u);
        SHA1_ROUND_2(b, c, d, e, a, 39u);
        SHA1_ROUND_3(a, b, c, d, e, 40u);
        SHA1_ROUND_3(e, a, b, c, d, 41u);
        SHA1_ROUND_3(d, e, a, b, c, 42u);
        SHA1_ROUND_3(c, d, e, a, b, 43u);
        SHA1_ROUND_3(b, c, d, e, a, 44u);
        SHA1_ROUND_3(a, b, c, d, e, 45u);
        SHA1_ROUND_3(e, a, b, c, d, 46u);
        SHA1_ROUND_3(d, e, a, b, c, 47u);
        SHA1_ROUND_3(c, d, e, a, b, 48u);
        SHA1_ROUND_3(b, c, d, e, a, 49u);
        SHA1_ROUND_3(a, b, c, d, e, 50u);
        SHA1_ROUND_3(e, a, b, c, d, 51u);
        SHA1_ROUND_3(d, e, a, b, c, 52u);
        SHA1_ROUND_3(c, d, e, a, b, 53u);
        SHA1_ROUND_3(b, c, d, e, a, 54u);
        SHA1_ROUND_3(a, b, c, d, e, 55u);
        SHA1_ROUND_3(e, a, b, c, d, 56u);
        SHA1_ROUND_3(d, e, a, b, c, 57u);
        SHA1_ROUND_3(c, d, e, a, b, 58u);
        SHA1_ROUND_3(b, c, d, e, a, 59u);
        SHA1_ROUND_4(a, b, c, d, e, 60u);
        SHA1_ROUND_4(e, a, b, c, d, 61u);
        SHA1_ROUND_4(d, e, a, b, c, 62u);
        SHA1_ROUND_4(c, d, e, a, b, 63u);
        SHA1_ROUND_4(b, c, d, e, a, 64u);
        SHA1_ROUND_4(a, b, c, d, e, 65u);
        SHA1_ROUND_4(e, a, b, c, d, 66u);
        SHA1_ROUND_4(d, e, a, b, c, 67u);
        SHA1_ROUND_4(c, d, e, a, b, 68u);
        SHA1_ROUND_4(b, c, d, e, a, 69u);
        SHA1_ROUND_4(a, b, c, d, e, 70u);
        SHA1_ROUND_4(e, a, b, c, d, 71u);
        SHA1_ROUND_4(d, e, a, b, c, 72u);
        SHA1_ROUND_4(c, d, e, a, b, 73u);
        SHA1_ROUND_4(b, c, d, e, a, 74u);
        SHA1_ROUND_4(a, b, c, d, e, 75u);
        SHA1_ROUND_4(e, a, b, c, d, 76u);
        SHA1_ROUND_4(d, e, a, b, c, 77u);
        SHA1_ROUND_4(c, d, e, a, b, 78u);
        SHA1_ROUND_4(b, c, d, e, a, 79u);

#undef SHA1_LOAD
#undef SHA1_ROUND_0
#undef SHA1_ROUND_1
#undef SHA1_ROUND_2
#undef SHA1_ROUND_3
#undef SHA1_ROUND_4

        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
    }

public:

    uint32_t state[5] { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 };
    uint8_t buf[64];
    uint32_t bi;
    uint64_t n_bits;

    explicit sha1(const std::string& text = ""): buf(), bi(0), n_bits(0) {
        add(text.c_str(), text.length());
    }

    sha1& add(uint8_t x) {
        add_byte_dont_count_bits(x);
        n_bits += 8;
        return *this;
    }

    sha1& add(void const * data, uint32_t n) {
        if (data == nullptr) return *this;
        const auto *ptr = reinterpret_cast<uint8_t const *>(data);

        // fill up block if not full
        for (; n && bi % sizeof(buf); n--) add(*ptr++);

        // process full blocks
        for (; n >= sizeof(buf); n -= sizeof(buf)) {
            process_block(ptr);
            ptr += sizeof(buf);
            n_bits += sizeof(buf) * 8;
        }

        // process remaining part of block
        for (; n; n--) add(*ptr++);

        return *this;
    }

    sha1& finalize() {
        // hashed text ends with 0x80, some padding 0x00 and the length in bits
        add_byte_dont_count_bits(0x80);
        while (bi % 64 != 56) add_byte_dont_count_bits(0x00);
        for (int j = 7; j >= 0; j--) add_byte_dont_count_bits(n_bits >> j * 8u);

        return *this;
    }

    std::string get(bool lower_case = true) {
        std::string ret;
        const char *alphabet = lower_case ? "0123456789abcdef" : "0123456789ABCDEF";
        for (uint32_t st : state) {
            for (int j = 7; j >= 0; j--) {
                ret += alphabet[(st >> j * 4u) & 0xfu];
            }
        }
        return ret;
    }
};


std::string sha1sum(const std::string& data, bool lower_case=true) {
    sha1 s(data);
    s.finalize();
    return s.get(lower_case);
}

#endif //NTORRENT_SHA1_H
