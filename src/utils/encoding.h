//
// Created by julian on 10.07.2020.
//

#ifndef NTORRENT_ENCODING_H
#define NTORRENT_ENCODING_H

#include <string>
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

#endif //NTORRENT_ENCODING_H
