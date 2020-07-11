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

namespace {
    inline bool is_url_acceptable(char c) {
        return ('a' <= c and c <= 'z') or ('A' <= c and c <= 'Z') or ('0' <= c and c <= '9')
               or c == '.' or c == '-' or c == '_' or c == '~';
    }

    inline bool is_hex_char(char c) {
        return ('0' <= c and c <= '9') or ('a' <= c and c <= 'f') or ('A' <= c and c <= 'F');
    }

    inline char hex_to_char(char c1, char c2) {
        std::stringstream ss;
        uint32_t ret = 0;
        ss << c1 << c2;
        ss >> std::hex >> ret;
        return static_cast<char>(ret);
    }
}


bool is_url_encoded(const string& data) {
    unsigned i = 0;
    while (i < data.size()) {
        if (data[i] == '%') {
            if (data.size() - i < 3) return false;
            if (not is_hex_char(data[i+1]) or not is_hex_char(data[i+2])) return false;
            i += 3;
        } else {
            if (not is_url_acceptable(data[i])) return false;
            i++;
        }
    }
    return true;
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


string url_encode_hash(string hex_hash) {
    if (hex_hash.size() % 2) throw std::domain_error("Hex hash needs to be of even length.");
    for (char c: hex_hash) if (not is_hex_char(c)) throw std::domain_error("Hex hash must be comprised of hex chars.");

    string ret;
    for (unsigned i = 0; i < hex_hash.size(); i += 2)
        ret += string("%") + hex_hash[i] + hex_hash[i+1];

    return ret;
}


#endif //NTORRENT_URL_ENCODING_H
