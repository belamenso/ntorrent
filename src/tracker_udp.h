//
// Created by julian on 11.07.2020.
//

#ifndef NTORRENT_TRACKER_UDP_H
#define NTORRENT_TRACKER_UDP_H

#include <string>
#include <cassert>
#include <netdb.h>
#include <unistd.h>
#include <optional>
#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <bits/socket.h>
using std::string, std::optional;

#include "utils/encoding.h"
#include "tracker_response.h"

namespace {
    unsigned extract_port(const string &name) {
        int semicolon_pos = static_cast<int>(name.length()) - 1;
        for (; semicolon_pos >= 0; semicolon_pos--) {
            if (name[semicolon_pos] == ':') break;
            if (not isdigit(name[semicolon_pos])) throw std::domain_error("Name must end with : and port.");
        }
        assert(semicolon_pos > 0);
        return semicolon_pos;
    }

    void write_all(const int fd, const char *data, unsigned n) {
        unsigned done = 0;
        while (done < n) {
            int got = write(fd, data + done, n);
            if (got == -1) {
                std::cerr << "??" << std::endl;
                return;
            }
            done += got;
        }
    }

    enum protocol_action {
        act_connect = 0,
        act_announce = 1,
        act_scrape = 2,
        act_error = 3
    };

    std::pair<string, uint32_t> udp_handshake() {
        uint32_t packet_32[4]{};
        packet_32[0] = htonl(uint32_t(0x417));
        packet_32[1] = htonl(uint32_t(0x27101980u));
        packet_32[2] = htonl(act_connect);
        packet_32[3] = random_uint32();

        auto ret = string(reinterpret_cast<char *>(packet_32), 16);
        return {ret, packet_32[3]};
    }

    vector<addrinfo> locate_udp_tracker(const string &name) {
        if (name.substr(0, string("udp://").size()) != "udp://")
            throw std::domain_error("Name must start with \"udp://\".");
        const unsigned name_pos = string("udp://").length();
        if (name.size() <= name_pos) throw std::domain_error("Nothing found after udp://");
        const unsigned semicolon_pos = extract_port(name);

        const string parsed_name = name.substr(name_pos, semicolon_pos - name_pos),
                parsed_port = name.substr(semicolon_pos + 1);

        vector<addrinfo> ret;

        {
            addrinfo *result;
            addrinfo hints{};
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = SOCK_DGRAM;
            hints.ai_flags = AI_NUMERICSERV;

            int error = getaddrinfo(parsed_name.c_str(), parsed_port.c_str(), &hints, &result);
            if (error != 0) {
                std::cerr << "getaddrinfo() failed: " << gai_strerror(error) << std::endl;
                return {};
            }

            for (auto r = result; r != nullptr; r = r->ai_next)
                ret.push_back(*r);

            freeaddrinfo(result);
        }

        return ret;
    }

    optional<std::pair<int, uint64_t>> handshake(const string &announce) {
        const vector<addrinfo> trackers = locate_udp_tracker(announce);
        if (trackers.empty()) return {};

        int sock_fd;
        const addrinfo *chosen = nullptr;
        for (const addrinfo &tracker: trackers) {
            if (-1 != (sock_fd = socket(tracker.ai_family, tracker.ai_socktype, tracker.ai_protocol))) {
                chosen = &tracker;
                break;
            }
        }
        if (chosen == nullptr) {
            std::cerr << "Could not open a socket." << std::endl;
            return {};
        }

        if (0 != connect(sock_fd, chosen->ai_addr, chosen->ai_addrlen)) {
            std::cerr << "connect() failed" << std::endl;
            return {};
        }

        const auto&[packet, tid] = udp_handshake();
        write_all(sock_fd, packet.c_str(), packet.size());

        uint8_t reply_8[4 * 4]{};
        int read_bytes = read(sock_fd, reply_8, sizeof(reply_8));
        if (read_bytes != sizeof(reply_8)) {
            std::cerr << "Managed to read only " << read_bytes << " bytes." << std::endl;
            return {};
        }
        auto *reply_32 = reinterpret_cast<uint32_t *>(reply_8);
        auto *reply_64 = reinterpret_cast<uint64_t *>(reply_8);

        if (reply_32[0] != act_connect) return {};
        if (reply_32[1] != tid) return {};

        auto connection_id = reply_64[1];
        return {{sock_fd, connection_id}};
    }
}

optional<tracker_scrape> udp_scrape(const string& announce, const vector<string>& info_hashes) {
    const auto handshake_result = handshake(announce);
    if (not handshake_result.has_value()) return {};
    const auto& [ sock_fd, connection_id ] = handshake_result.value();

    uint8_t packet_8[16 + 20 * info_hashes.size()];
    auto *packet_32 = reinterpret_cast<uint32_t*>(packet_8);
    auto *packet_64 = reinterpret_cast<uint64_t*>(packet_8);
    packet_64[0] = connection_id;
    packet_32[2] = htonl(act_scrape);
    const uint32_t transaction_id = random_uint32();
    packet_32[3] = transaction_id;

    for (unsigned i = 0; i < info_hashes.size(); i++) {
        if (info_hashes[i].size() != 20)
            throw std::domain_error("All correct info hashes must be of size 20.");
        for (unsigned j = 0; j < 20; j++)
            packet_8[16 + 20*i + j] = static_cast<uint8_t>(info_hashes[i][j]);
    }

    write_all(sock_fd, reinterpret_cast<const char *>(packet_8), sizeof(packet_8));

    uint8_t reply_8[8 + 12*info_hashes.size()];
    auto *reply_32 = reinterpret_cast<uint32_t*>(reply_8);

    int bytes_read = read(sock_fd, reply_8, sizeof(reply_8));
    if (bytes_read <= 0) {
        std::cerr << "Managed to read only " << bytes_read << " bytes." << std::endl;
        return {};
    }

    map<string, scrape_file> m;

    if (reply_32[0] != htonl(act_scrape)) return {};
    if (reply_32[1] != transaction_id) return {};
    if ((bytes_read - 8) % 12) return {};
    auto *reply_shifted_32 = reinterpret_cast<uint32_t*>(reply_8 + 16);
    for (unsigned i = 0; i < (bytes_read - 8)/12; i++) {
        uint32_t complete   = ntohl(reply_shifted_32[3*i+0]),
                 downloaded = ntohl(reply_shifted_32[3*i+1]),
                 incomplete = ntohl(reply_shifted_32[3*i+2]);
        m.insert({ info_hashes[i], { complete, downloaded, incomplete, {} }});
    }

    close(sock_fd);
    return { tracker_scrape( {}, m, {} ) };
}

#endif //NTORRENT_TRACKER_UDP_H
