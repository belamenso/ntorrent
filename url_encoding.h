//
// Created by julian on 09.07.2020.
//

#ifndef NTORRENT_URL_ENCODING_H
#define NTORRENT_URL_ENCODING_H

#include <string>
using std::string;
#include <sstream>
#include <optional>
using std::optional;


static inline bool is_url_acceptable(char c) {
    return ('a' <= c and c <= 'z') or ('A' <= c and c <= 'Z') or ('0' <= c and c <= '9')
           or c == '.' or c == '-' or c == '_' or c == '~';
}


string url_encode(const string& decoded) {
    std::stringstream encoded;
    encoded << std::hex;

    for (uint8_t c: decoded) {
        if (is_url_acceptable(c)) {
            encoded << static_cast<char>(c);
        } else {
            encoded << '%' << (c >> 4u) << (c & 0b1111u);
        }
    }

    return encoded.str();
}

static inline char hex_to_char(char c1, char c2) {
    std::stringstream ss;
    uint32_t ret = 0;
    ss << c1 << c2;
    ss >> std::hex >> ret;
    return static_cast<char>(ret);
}


static inline bool is_hex_char(char c) {
    return ('0' <= c and c <= '9') or ('a' <= c and c <= 'f') or ('A' <= c and c <= 'F');
}


optional<string> url_decode(const string& encoded) {
    std::stringstream decoded;
    int i = 0;
    while (i < encoded.size()) {
        if (encoded[i] == '%') {
            if (i + 2 >= encoded.size()) return {};
            if (not is_hex_char(encoded[i+1]) or not is_hex_char(encoded[i+2])) return {};
            decoded << hex_to_char(encoded[i+1], encoded[i+2]);
            i += 3;
            continue;
        } else {
            if (not is_url_acceptable(encoded[i])) return {};
            decoded << encoded[i];
            i++;
        }
    }
    return { decoded.str() };
}

#endif //NTORRENT_URL_ENCODING_H
