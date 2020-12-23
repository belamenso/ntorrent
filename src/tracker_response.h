//
// Created by julian on 10.07.2020.
//

#ifndef NTORRENT_TRACKER_RESPONSE_H
#define NTORRENT_TRACKER_RESPONSE_H

#include <map>
#include <string>
#include <vector>
#include <memory>
#include <ostream>
#include <variant>
#include <optional>
using std::string, std::optional, std::vector, std::map;
#include <arpa/inet.h>

#include "bencoding.h"
#include "utils/encoding.h"


struct peer {
    const optional<string> peer_id;
    const std::variant<uint32_t, __uint128_t> ip; // XXX net byte order
    const uint16_t port; // XXX net byte order

    peer(optional<string> peer_id, uint32_t ip, uint16_t port)
    : peer_id(std::move(peer_id)), ip(ip), port(port) {}

    peer(optional<string> peer_id, __uint128_t ip, uint16_t port)
    : peer_id(std::move(peer_id)), ip(ip), port(port) {}

    [[nodiscard]] bool is_ipv6() const { return std::holds_alternative<__uint128_t>(this->ip); }
    [[nodiscard]] bool is_ipv4() const { return std::holds_alternative<uint32_t>(this->ip); }

    friend std::ostream& operator << (std::ostream& os, const peer& p) {
        os << "peer[ip: ";
        if (std::holds_alternative<uint32_t>(p.ip)) os << ip_to_str(std::get<uint32_t>(p.ip));
        else os << ip_to_str(std::get<__uint128_t>(p.ip));
        os << ", port: " << port_to_str(p.port);
        if (p.peer_id.has_value()) os << ", peer id: " << url_encode(p.peer_id.value());
        os << "]";
        return os;
    }
};


struct tracker_response {
    const optional<string> warning_message;
    const uint32_t interval;
    const optional<uint32_t> min_interval;
    const optional<string> tracker_id;
    const optional<uint32_t> complete;
    const optional<uint32_t> incomplete;
    const vector<peer> peers;


    tracker_response(optional<string> warning_message, uint32_t interval, optional<uint32_t> min_interval,
            optional<string> tracker_id, optional<uint32_t> complete, optional<uint32_t> incomplete, vector<peer> peers)
            : warning_message(std::move(warning_message)), interval(interval), min_interval(min_interval),
            tracker_id(std::move(tracker_id)), complete(complete), incomplete(incomplete), peers(std::move(peers)) {}


    friend std::ostream& operator << (std::ostream& os, const tracker_response& r) {
        os << "tracker_response[\n"
           << (r.warning_message.has_value() ? "  warning message: " + r.warning_message.value() + "\n" : "")
           << "  interval: " << seconds_to_readable_str(r.interval) << "\n"
           << (r.min_interval.has_value() ? "  min interval: " + seconds_to_readable_str(r.min_interval.value()) + "\n" : "")
           << (r.tracker_id.has_value() ? "  tracker id: " + r.tracker_id.value() + "\n" : "")
           << (r.complete.has_value() ? "  complete: " + std::to_string(r.complete.value()) + "\n" : "")
           << (r.incomplete.has_value() ? "  incomplete: " + std::to_string(r.incomplete.value()) + "\n" : "")
           << "  peers: [\n";
        for (const peer& p: r.peers) os << "    " << p << "\n";
        os << "  ]\n"
           << "]";
        return os;
    }


    static optional<std::variant<string, tracker_response>> parse(const std::shared_ptr<bnode>& root) {
        const auto& dict = *dynamic_cast<bdictionary*>(root.get());

        if (dict.has("failure reason", bstring_t))
            return { dict.get_string("failure reason") };

        optional<string> warning_message;
        uint32_t interval;
        optional<uint32_t> min_interval;
        optional<string> tracker_id;
        optional<uint32_t> complete;
        optional<uint32_t> incomplete;
        vector<peer> peers;

        if (dict.has("warning message", bstring_t))
            warning_message = { dict.get_string("warning message").value() };

        if (not dict.has("interval", bint_t)) return {};
        interval = dict.get_int("interval").value();

        if (dict.has("min interval", bint_t))
            min_interval = { dict.get_int("min interval").value() };

        if (dict.has("tracker id", bstring_t))
            tracker_id = { dict.get_string("tracker id").value() };

        if (dict.has("complete", bint_t))
            complete = { dict.get_int("complete").value() };

        if (dict.has("incomplete", bint_t))
            incomplete = { dict.get_int("incomplete").value() };

        if (dict.has("peers", blist_t)) {
            blist* peers_list_ptr = dynamic_cast<blist*>(dict.dict.at(bstring("peers")).get());
            assert (peers_list_ptr != nullptr);
            for (const auto& peer: peers_list_ptr->elements) {
                const auto& peer_dict = *dynamic_cast<bdictionary*>(peer.get());
                string peer_id, ip_str;
                uint16_t port;

                if (not peer_dict.has("peer id", bstring_t)) return {};
                peer_id = peer_dict.get_string("peer id").value();

                if (not peer_dict.has("ip", bstring_t)) return {};
                ip_str = peer_dict.get_string("ip").value();

                if (not peer_dict.has("port", bint_t)) return {};
                port = peer_dict.get_int("port").value();

                peers.emplace_back( optional<string>(peer_id), ip_string_to_ip(ip_str), port );
            }
        } else if (dict.has("peers", bstring_t)) {
            bstring* peers_str_ptr = dynamic_cast<bstring*>(dict.dict.at(bstring("peers")).get());
            assert(peers_str_ptr != nullptr);
            const string& peers_str = peers_str_ptr->value;
            if (peers_str.size() % (4+2)) return {};

            for (unsigned i = 0; i < peers_str.size(); i += (4+2)) {
                const uint32_t ip   = *reinterpret_cast<const uint32_t*>(peers_str.c_str() + i);
                const uint16_t port = *reinterpret_cast<const uint16_t*>(peers_str.c_str() + i + 4);
                peers.emplace_back( optional<string>(), ip, port );
            }
        }

        if (dict.has("peers6", bstring_t)) {
            bstring* peers6_str_ptr = dynamic_cast<bstring*>(dict.dict.at(bstring("peers6")).get());
            assert(peers6_str_ptr != nullptr);
            const string& peers6_str = peers6_str_ptr->value;
            if (peers6_str.size() % (16+2)) return {};

            for (unsigned i = 0; i < peers6_str.size(); i += (16+2)) {
                const __uint128_t ip6 = *reinterpret_cast<const __uint128_t*>(peers6_str.c_str() + i);
                const uint16_t port   = *reinterpret_cast<const uint16_t*>(peers6_str.c_str() + i + 16);
                peers.emplace_back( optional<string>(), ip6, port );
            }
        }

        if (peers.empty()) return {};

        return { tracker_response( warning_message, interval, min_interval, tracker_id, complete, incomplete, peers ) };
    };

};


struct scrape_file {
    const uint32_t complete, downloaded, incomplete;
    const optional<string> name;

    scrape_file(scrape_file const &f) = default;

    scrape_file(uint32_t complete, uint32_t downloaded, uint32_t incomplete, optional<string> name)
    : complete(complete), downloaded(downloaded), incomplete(incomplete), name(std::move(name)) {}

    friend std::ostream& operator << (std::ostream& os, const scrape_file& f) {
        os << "scrape_file["
           << (f.name.has_value() ? "name: " + f.name.value() + ", " : "")
           << "complete: " << f.complete << ", downloaded: " << f.downloaded << ", incomplete: " << f.incomplete
           << "]";
        return os;
    }

};


struct tracker_scrape {
    const optional<string> failure_reason; // if this is set, all the other fields are ignored
    const map<string, scrape_file> files;
    const optional<uint32_t> flag_min_request_interval;

    tracker_scrape(optional<string> failure_reason, map<string, scrape_file> files, optional<uint32_t> flag_min_request_interval)
    : failure_reason(std::move(failure_reason)), files(std::move(files)), flag_min_request_interval(flag_min_request_interval) {}

    friend std::ostream& operator << (std::ostream& os, const tracker_scrape& t) {
        os << "scrape[\n";
        if (t.failure_reason.has_value()) {
            os << "  failure reason: " << t.failure_reason.value() << "\n]";
            return os;
        }
        os << (t.flag_min_request_interval.has_value() ?
                "  flag min_request_interval: " + std::to_string(t.flag_min_request_interval.value()) + "\n" : "")
           << "  files: [\n";
        for (const auto& [k, v]: t.files)
            os << "    " << bin_to_hex_string(reinterpret_cast<const uint8_t *>(k.c_str()), k.length()) << ": \n"
               << "      " << v << "\n";
        os << "  ]\n]";
        return os;
    }

    static optional<tracker_scrape> parse(const std::shared_ptr<bnode>& root) {
        const auto& dict = *dynamic_cast<bdictionary*>(root.get());

        if (dict.has("failure reason", bstring_t))
            return { tracker_scrape({dict.get_string("failure reason").value()}, {}, {}) };

        map<string, scrape_file> files;
        optional<uint32_t> flag_min_request_interval;

        if (dict.has("flags", bdictionary_t)) {
            const auto& flags_dict = *dynamic_cast<bdictionary*>(dict.dict.at(bstring("flags")).get());
            if (flags_dict.has("min_request_interval", bint_t)) {
                flag_min_request_interval = { flags_dict.get_int("min_request_interval").value() };
            }
        }

        if (not dict.has("files", bdictionary_t)) return {};
        const auto& files_dict = *dynamic_cast<bdictionary*>(dict.dict.at(bstring("files")).get());
        for (const auto& [ih, fd]: files_dict.dict) {
            uint32_t complete, downloaded, incomplete;
            optional<string> name;

            const auto& file_dict = *dynamic_cast<bdictionary*>(fd.get());

            if (not file_dict.has("complete", bint_t)) return {};
            complete = file_dict.get_int("complete").value();

            if (not file_dict.has("downloaded", bint_t)) return {};
            downloaded = file_dict.get_int("downloaded").value();

            if (not file_dict.has("incomplete", bint_t)) return {};
            incomplete = file_dict.get_int("incomplete").value();

            if (file_dict.has("name", bstring_t))
                name = file_dict.get_string("name").value();

            if (0 != files.count(ih.value)) return {};
            files.insert({ih.value, std::move(scrape_file( complete, downloaded, incomplete, name ))});
        }

        return {{ optional<string>(), files, flag_min_request_interval }};
    }
};


#endif //NTORRENT_TRACKER_RESPONSE_H
