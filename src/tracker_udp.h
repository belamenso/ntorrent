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
#include <netinet/tcp.h>

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

    enum protocol_action {
        act_connect = 0,
        act_announce = 1,
        act_scrape = 2,
        act_error = 3
    };

    template <class T> T& at(void* data, size_t offset, size_t index=0) {
        return reinterpret_cast<T*>( reinterpret_cast<uint8_t*>(data) + offset )[index];
    }

    std::pair<string, uint32_t> udp_handshake() {
        uint32_t packet_32[4]{};
        at<uint64_t>(packet_32, 0, 0) = htonll(0x41727101980ull);
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
        write(sock_fd, packet.c_str(), packet.size());

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

    map<string, scrape_file> m;

    uint8_t packet[16 + 20 * info_hashes.size()];
    at<uint64_t>(packet, 0) = connection_id;
    at<uint32_t>(packet, 0, 2) = htonl(act_scrape);
    const uint32_t transaction_id = random_uint32();
    at<uint32_t>(packet, 0, 3) = transaction_id;

    for (unsigned i = 0; i < info_hashes.size(); i++) {
        if (info_hashes[i].size() != 20)
            throw std::domain_error("All correct info hashes must be of size 20.");
        for (unsigned j = 0; j < 20; j++)
            packet[16 + 20*i + j] = static_cast<uint8_t>(info_hashes[i][j]);
    }

    write(sock_fd, reinterpret_cast<const char *>(packet), sizeof(packet));

    uint8_t reply[8 + 12*info_hashes.size()];

    int bytes_read = read(sock_fd, reply, sizeof(reply));
    if (bytes_read <= 0) {
        std::cerr << "Managed to read only " << bytes_read << " bytes." << std::endl;
        goto failure;
    }

    if (at<uint32_t>(reply, 0, 0) != htonl(act_scrape)) goto failure;
    if (at<uint32_t>(reply, 0, 1) != transaction_id) goto failure;
    if ((bytes_read - 8) % 12) goto failure;
    for (unsigned i = 0; i < (bytes_read - 8)/12; i++) {
        uint32_t complete   = ntohl(at<uint32_t>(reply, 16, 3*i+0)),
                 downloaded = ntohl(at<uint32_t>(reply, 16, 3*i+1)),
                 incomplete = ntohl(at<uint32_t>(reply, 16, 3*i+2));
        m.insert({ info_hashes[i], { complete, downloaded, incomplete, {} }});
    }

    close(sock_fd);
    return { tracker_scrape( {}, m, {} ) };

failure:
    close(sock_fd);
    return {};
}

optional<tracker_response> udp_announce(const tracker_request& req) {
    const auto handshake_result = handshake(req.announce);
    if (not handshake_result.has_value()) return {};
    const auto& [ sock_fd, connection_id ] = handshake_result.value();

    in_addr addr{};
    if (req.ip.has_value()) {
        int err = inet_pton(AF_INET, req.ip.value().c_str(), &addr);
        if (err != 1) close(sock_fd);
        if (err == 0) throw std::domain_error("IP address not in presentation format");
        if (err != 1) throw std::domain_error("Cannot convert IP address");
    }

    uint8_t packet[100];
    at<uint64_t>(packet, 0) = connection_id;
    at<uint32_t>(packet, 0, 2) = htonl(act_announce);
    const uint32_t transaction_id = random_uint32();
    at<uint32_t>(packet, 0, 3) = transaction_id;
    for (unsigned i = 0; i < 20; i++) packet[16+i] = req.info_hash[i];
    for (unsigned i = 0; i < 20; i++) packet[16+20+i] = req.peer_id[i];
    at<uint64_t>(packet, 16+40, 0) = htonll(req.downloaded);
    at<uint64_t>(packet, 16+40, 1) = htonll(req.left);
    at<uint64_t>(packet, 16+40, 2) = htonll(req.uploaded);
    at<uint32_t>(packet, 16+40+24, 0) = htonll(
            (!req.event.has_value())? 0
                : (req.event.value() == completed)? 1
                : (req.event.value() == started)? 2 : 3);
    at<uint32_t>(packet, 16+40+24, 1) = req.ip.has_value() ? addr.s_addr : htonl(0);
    at<uint32_t>(packet, 16+40+24, 2) = req.key.has_value() ? req.key.value() : random_uint32();
    at<uint32_t>(packet, 16+40+24, 3) = req.numwant.has_value() ? htonl(req.numwant.value()) : htons(-1);
    at<uint16_t>(packet, 16+40+24+16, 0) = htons(req.port);
    at<uint16_t>(packet, 16+40+24+16, 1) = 0; // TODO extensions not supported

    write(sock_fd, reinterpret_cast<const char *>(packet), sizeof(packet));

    vector<peer> peers;

    uint8_t reply[5*4+100*6];
    int bytes_read = read(sock_fd, reply, sizeof(reply));
    if (bytes_read <= 0) {
        std::cerr << "Managed to read only " << bytes_read << " bytes." << std::endl;
        goto failure;
    }

    if (bytes_read < 20) return {};
    if ((bytes_read - 20) % 6) return {};
    if (at<uint32_t>(reply, 0, 0) != htonl(act_announce)) return {};
    if (at<uint32_t>(reply, 0, 1) != transaction_id) return {};
    for (unsigned i = 0; i < (bytes_read - 20)/6; i++)
        peers.emplace_back( optional<string>(),
                            at<uint32_t>(reply, 20+6*i),
                            at<uint16_t>(reply, 20 + 6*i + 4) );

    close(sock_fd);
    return { tracker_response({}, ntohl(at<uint32_t>(reply, 0, 2)),
                                {}, {}, ntohl(at<uint32_t>(reply, 0, 4)),
                                ntohl(at<uint32_t>(reply, 0, 3)), peers) };

failure:
    close(sock_fd);
    return {};
}

#endif //NTORRENT_TRACKER_UDP_H
