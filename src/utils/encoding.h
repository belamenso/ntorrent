//
// Created by julian on 10.07.2020.
//

#ifndef NTORRENT_ENCODING_H
#define NTORRENT_ENCODING_H

#include <string>
#include <random>
#include <iomanip>

using std::string;

string bin_to_hex_string(uint8_t const * data, unsigned size, bool lower_case=true) {
    const char* alphabet = lower_case? "0123456789abcdef" : "0123456789ABCDEF";

    string ret(2*size, '\0');
    for (unsigned i = 0; i < size; i++) {
        const uint8_t c = data[i];
        ret[2*i+0] = alphabet[c / 16u];
        ret[2*i+1] = alphabet[c & 15u];
    }
    return ret;
}

string bin_to_hex_string(const string& str, bool lower_case=true) {
    return bin_to_hex_string(reinterpret_cast<const uint8_t *>(str.c_str()), str.size(), lower_case);
}

string bin_ip_to_string_ip(const uint8_t* data) {
    string ip;
    for (unsigned j = 0; j < 4; j++)
        ip += std::to_string(data[j]) + ".";
    ip.pop_back();
    return ip;
}

[[nodiscard]] string random_bytes(unsigned size) {
    std::random_device engine;
    string ret(size, '\0');
    for (unsigned i = 0; i < size; i++) ret[i] = engine();
    return ret;
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

string seconds_to_readable_str(unsigned seconds) {
    const unsigned h = seconds / 60 / 60,
                   m = (seconds / 60) % 60,
                   s = seconds % 60;
    string ret;
    if (h) ret += std::to_string(h) + "h ";
    if (m) ret += std::to_string(m) + "m ";
    if (s) ret += std::to_string(s) + "s ";
    if (ret.empty()) return "0s";
    ret.pop_back();
    return ret;
}

uint32_t ip_string_to_ip(const string& ip) {
    in_addr addr{};
    int err = inet_pton(AF_INET, ip.c_str(), &addr);
    if (err == 0) throw std::domain_error("IP address not in presentation format");
    if (err != 1) throw std::domain_error("Cannot convert IP address");
    return addr.s_addr;
}

string human_readable_size(uint64_t n) {
    const string size_str[] { "B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB" };
    uint64_t sizes[] { n, 0, 0, 0, 0, 0, 0 };
    for (unsigned i = 1; i < 7; i++)
        sizes[i] = sizes[i-1] >> 10u;
    for (int i = 6; i >= 0; i--) {
        if (sizes[i]) {
            std::stringstream ss;
            ss << std::fixed;
            const double r = n/pow(1024,i);
            ss << std::setprecision(round(r) == r ? 0 : 1) << r << ' ' << size_str[i];
            if (i) ss << " (" << n << " B)";
            return ss.str();
        }
    }
    return "0 B";
}

#endif //NTORRENT_ENCODING_H
