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

[[nodiscard]] uint32_t random_uint32() {
    std::random_device engine;
    uint8_t ret[4]{};
    for (unsigned char &i : ret) i = engine();
    return *reinterpret_cast<uint32_t*>(ret);
}

inline uint64_t htonll(uint64_t x) {
    return ((((uint64_t)htonl(x)) << 32u) + htonl(x >> 32u));
}

inline uint64_t ntohll(uint64_t x) {
    return htonll(x);
}

string ip_to_str(uint32_t ip) {
    string ret;
    const auto* bytes = reinterpret_cast<uint8_t*>(&ip);
    for (int i = 0; i < 4; i++)
        ret += std::to_string(bytes[i]) + ".";
    ret.pop_back();
    return ret;
}

string port_to_str(uint16_t port) {
    return std::to_string(ntohs(port));
}

uint32_t str_to_u32(const string& str) {
    if (str.size() != 4) throw std::domain_error("Exactly 4 bytes needed");
    char bytes[4] { str[0], str[1], str[2], str[3] };
    return *reinterpret_cast<uint32_t*>(bytes);
}

#endif //NTORRENT_ENCODING_H
