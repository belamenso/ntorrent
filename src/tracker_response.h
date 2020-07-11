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
#include "encoding.h"


struct peer {
    const optional<string> peer_id;
    const string ip;
    const uint64_t port;

    peer(optional<string> peer_id, string ip, uint64_t port)
    : peer_id(std::move(peer_id)), ip(std::move(ip)), port(port) {}

    friend std::ostream& operator << (std::ostream& os, const peer& p) {
        os << "peer[ip: " << p.ip << ", port: " << p.port;
        if (p.peer_id.has_value()) os << ", peer id: " << url_encode(p.peer_id.value());
        os << "]";
        return os;
    }
};


struct tracker_response {
    const optional<string> warning_message;
    const uint64_t interval;
    const optional<uint64_t> min_interval;
    const optional<string> tracker_id;
    const uint64_t complete;
    const uint64_t incomplete;
    const vector<peer> peers;


    tracker_response(optional<string> warning_message, uint64_t interval, optional<uint64_t> min_interval,
            optional<string> tracker_id, uint64_t complete, uint64_t incomplete, vector<peer> peers)
            : warning_message(std::move(warning_message)), interval(interval), min_interval(min_interval),
            tracker_id(std::move(tracker_id)), complete(complete), incomplete(incomplete), peers(std::move(peers)) {}


    friend std::ostream& operator << (std::ostream& os, const tracker_response& r) {
        os << "tracker_response[\n"
           << (r.warning_message.has_value() ? "  warning message: " + r.warning_message.value() + "\n" : "")
           << "  interval: " << r.interval << "\n"
           << (r.min_interval.has_value() ? "  min interval: " + std::to_string(r.min_interval.value()) + "\n" : "")
           << (r.tracker_id.has_value() ? "  tracker id: " + r.tracker_id.value() + "\n" : "")
           << "  complete: " << r.complete << "\n"
           << "  incomplete: " << r.incomplete << "\n"
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
        uint64_t interval;
        optional<uint64_t> min_interval;
        optional<string> tracker_id;
        uint64_t complete;
        uint64_t incomplete;
        vector<peer> peers;

        if (dict.has("warning message", bstring_t))
            warning_message = { dict.get_string("warning message").value() };

        if (not dict.has("interval", bint_t)) return {};
        interval = dict.get_int("interval").value();

        if (dict.has("min interval", bint_t))
            min_interval = { dict.get_int("min interval").value() };

        if (dict.has("tracker id", bstring_t))
            tracker_id = { dict.get_string("tracker id").value() };

        if (not dict.has("complete", bint_t)) return {};
        complete = dict.get_int("complete").value();

        if (not dict.has("incomplete", bint_t)) return {};
        incomplete = dict.get_int("incomplete").value();

        if (dict.has("peers", blist_t)) {
            blist* peers_list_ptr = dynamic_cast<blist*>(dict.dict.at(bstring("peers")).get());
            assert (peers_list_ptr != nullptr);
            for (const auto& peer: peers_list_ptr->elements) {
                const auto& peer_dict = *dynamic_cast<bdictionary*>(peer.get());
                string peer_id, ip;
                uint64_t port;

                if (not peer_dict.has("peer id", bstring_t)) return {};
                peer_id = peer_dict.get_string("peer id").value();

                if (not peer_dict.has("ip", bstring_t)) return {};
                ip = peer_dict.get_string("ip").value();

                if (not peer_dict.has("port", bint_t)) return {};
                port = peer_dict.get_int("port").value();

                peers.emplace_back( optional<string>(peer_id), ip, port );
            }
        } else if (dict.has("peers", bstring_t)) {
            bstring* peers_str_ptr = dynamic_cast<bstring*>(dict.dict.at(bstring("peers")).get());
            assert(peers_str_ptr != nullptr);
            const string& peers_str = peers_str_ptr->value;
            if (peers_str.size() % 6) return {};

            for (unsigned i = 0; i < peers_str.size(); i += 6) {
                string ip;
                for (unsigned j = 0; j < 4; j++) ip += std::to_string(static_cast<uint8_t>(peers_str[i+j])) + ".";
                ip.pop_back();

                uint64_t port = static_cast<uint8_t>(peers_str[i+4]);
                port = (port <<= 8u) + static_cast<uint8_t>(peers_str[i+5]);

                peers.emplace_back( optional<string>(), ip, port );
            }
        } else {
            return {};
        }

        return { tracker_response( warning_message, interval, min_interval, tracker_id, complete, incomplete, peers ) };
    };

};


struct scrape_file {
    const uint64_t complete, downloaded, incomplete;
    const optional<string> name;

    scrape_file(scrape_file const &f) = default;

    scrape_file(uint64_t complete, uint64_t downloaded, uint64_t incomplete, optional<string> name)
    : complete(complete), downloaded(downloaded), incomplete(incomplete), name(std::move(name)) {}

    friend std::ostream& operator << (std::ostream& os, const scrape_file& f) {
        os << "scrape_file["
           << (f.name.has_value() ? "name: " + f.name.value() + ", " : "")
           << "complete: " << f.complete << ", downloaded: " << f.downloaded << ", incomplate: " << f.incomplete
           << "]";
        return os;
    }

};


struct tracker_scrape {
    const optional<string> failure_reason; // if this is set, all the other fields are ignored
    const map<string, scrape_file> files;
    const optional<uint64_t> flag_min_request_interval;

    tracker_scrape(optional<string> failure_reason, map<string, scrape_file> files, optional<uint64_t> flag_min_request_interval)
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
        optional<uint64_t> flag_min_request_interval;

        if (dict.has("flags", bdictionary_t)) {
            const auto& flags_dict = *dynamic_cast<bdictionary*>(dict.dict.at(bstring("flags")).get());
            if (flags_dict.has("min_request_interval", bint_t)) {
                flag_min_request_interval = { flags_dict.get_int("min_request_interval").value() };
            }
        }

        if (not dict.has("files", bdictionary_t)) return {};
        const auto& files_dict = *dynamic_cast<bdictionary*>(dict.dict.at(bstring("files")).get());
        for (const auto& [ih, fd]: files_dict.dict) {
            uint64_t complete, downloaded, incomplete;
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
