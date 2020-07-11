//
// Created by julian on 10.07.2020.
//

#ifndef NTORRENT_ENCODING_H
#define NTORRENT_ENCODING_H

#include <string>
#include <random>

using std::string;

string bin_to_hex_string(uint8_t const * data, unsigned size, bool lower_case=true) {
    const char* alphabet = lower_case? "0123456789abcdef" : "0123456789ABCDEF";

    string ret;
    for (unsigned i = 0; i < size; i++) {
        const uint8_t c = data[i];
        ret += alphabet[c / 16u];
        ret += alphabet[c & 15u];
    }
    return ret;
}

string bin_ip_to_string_ip(const uint8_t* data) {
    string ip;
    for (unsigned j = 0; j < 4; j++)
        ip += std::to_string(data[j]) + ".";
    ip.pop_back();
    return ip;
}

uint16_t bin_port_to_uint16(const uint8_t* data) {
    return static_cast<uint16_t>( (data[0] << 8u) + data[1] );
}

[[nodiscard]] uint32_t random_uint32() {
    std::random_device engine;
    uint8_t ret[4]{};
    for (unsigned char &i : ret) i = engine();
    return *reinterpret_cast<uint32_t*>(ret);
}

string net_bytes(uint32_t x) {
    uint32_t n = htonl(x);
    auto data = reinterpret_cast<char*>( reinterpret_cast<uint8_t*>(&n) );
    return string(data, 4);
}

uint64_t htonll(uint64_t x) {
    const auto* data = reinterpret_cast<uint32_t*>(&x);
    return (uint64_t(htonl(data[1])) << 32u) + htonl(data[0]);
}

uint64_t ntohll(uint64_t x) {
    return htonll(x);
}

#endif //NTORRENT_ENCODING_H
