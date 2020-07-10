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
           << "  complate: " << r.complete << "\n"
           << "  incomplete: " << r.incomplete << "\n"
           << "  peers: [\n";
        for (const peer& p: r.peers) os << "    " << p << "\n";
        os << "  ]\n"
           << "]";
        return os;
    }


    static optional<std::variant<string, tracker_response>> parse(const std::shared_ptr<BNode>& root) {
        const auto& dict = dynamic_cast<BDictionary*>(root.get())->dict;

        if (1 == dict.count(BString("failure reason")))
            return { dynamic_cast<BString*>(dict.at(BString("failure reason")).get())->value };

        optional<string> warning_message;
        uint64_t interval;
        optional<uint64_t> min_interval;
        optional<string> tracker_id;
        uint64_t complete;
        uint64_t incomplete;
        vector<peer> peers;

        if (1 == dict.count(BString("warning message")))
            warning_message = { dynamic_cast<BString*>(dict.at(BString("warning message")).get())->value };

        if (0 == dict.count(BString("interval"))) return {};
        interval = dynamic_cast<BInt*>(dict.at(BString("interval")).get())->value;

        if (1 == dict.count(BString("min interval")))
            min_interval = { dynamic_cast<BInt*>(dict.at(BString("min interval")).get())->value };

        if (1 == dict.count(BString("tracker id")))
            tracker_id = { dynamic_cast<BString*>(dict.at(BString("tracker id")).get())->value };

        if (0 == dict.count(BString("complete"))) return {};
        complete = dynamic_cast<BInt*>(dict.at(BString("complete")).get())->value;

        if (0 == dict.count(BString("incomplete"))) return {};
        incomplete = dynamic_cast<BInt*>(dict.at(BString("incomplete")).get())->value;

        if (0 == dict.count(BString("peers"))) return {};

        BList* peers_list_ptr = dynamic_cast<BList*>(dict.at(BString("peers")).get());
        if (peers_list_ptr != nullptr) { /// XXX dictionary model
            for (const auto& peer: peers_list_ptr->elements) {
                const auto& peer_dict = dynamic_cast<BDictionary*>(peer.get())->dict;
                string peer_id, ip;
                uint64_t port;

                if (0 == peer_dict.count(BString("peer id"))) return {};
                peer_id = dynamic_cast<BString*>(peer_dict.at(BString("peer id")).get())->value;

                if (0 == peer_dict.count(BString("ip"))) return {};
                ip = dynamic_cast<BString*>(peer_dict.at(BString("ip")).get())->value;

                if (0 == peer_dict.count(BString("port"))) return {};
                port = dynamic_cast<BInt*>(peer_dict.at(BString("port")).get())->value;

                peers.emplace_back( optional<string>(peer_id), ip, port );
            }
        } else { /// XXX binary model
            BString* peers_str_ptr = dynamic_cast<BString*>(dict.at(BString("peers")).get());
            if (peers_str_ptr == nullptr) return {};
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

    static optional<tracker_scrape> parse(const std::shared_ptr<BNode>& root) {
        const auto& dict = dynamic_cast<BDictionary*>(root.get())->dict;

        if (1 == dict.count(BString("failure reason"))) {
            string failure_reason = dynamic_cast<BString*>(dict.at(BString("failure reason")).get())->value;
            return { tracker_scrape({failure_reason}, {}, {}) };
        }

        map<string, scrape_file> files;
        optional<uint64_t> flag_min_request_interval;

        if (1 == dict.count(BString("flags"))) {
            const auto& flags_dict = dynamic_cast<BDictionary*>(dict.at(BString("flags")).get())->dict;
            if (1 == flags_dict.count(BString("min_request_interval"))) {
                flag_min_request_interval =
                        { dynamic_cast<BInt*>(flags_dict.at(BString("min_request_interval")).get())->value };
            }
        }

        if (0 == dict.count(BString("files"))) return {};
        const auto& files_dict = dynamic_cast<BDictionary*>(dict.at(BString("files")).get())->dict;
        for (const auto& [ih, fd]: files_dict) {
            uint64_t complete, downloaded, incomplete;
            optional<string> name;

            const auto& file_dict = dynamic_cast<BDictionary*>(fd.get())->dict;

            if (0 == file_dict.count(BString("complete"))) return {};
            complete = dynamic_cast<BInt*>(file_dict.at(BString("complete")).get())->value;

            if (0 == file_dict.count(BString("downloaded"))) return {};
            downloaded = dynamic_cast<BInt*>(file_dict.at(BString("downloaded")).get())->value;

            if (0 == file_dict.count(BString("incomplete"))) return {};
            incomplete = dynamic_cast<BInt*>(file_dict.at(BString("incomplete")).get())->value;

            if (1 == file_dict.count(BString("name")))
                name = { dynamic_cast<BString*>(file_dict.at(BString("name")).get())->value };

            if (0 != files.count(ih.value)) return {};
            files.insert({ih.value, std::move(scrape_file( complete, downloaded, incomplete, name ))});
        }

        return {{ optional<string>(), files, flag_min_request_interval }};
    }
};


#endif //NTORRENT_TRACKER_RESPONSE_H
