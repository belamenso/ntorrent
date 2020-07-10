//
// Created by julian on 10.07.2020.
//

#ifndef NTORRENT_TRACKER_RESPONSE_H
#define NTORRENT_TRACKER_RESPONSE_H

#include <string>
#include <vector>
#include <memory>
#include <ostream>
#include <variant>
#include <optional>
using std::string, std::optional, std::vector;
#include <arpa/inet.h>

#include "bencoding.h"


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


#endif //NTORRENT_TRACKER_RESPONSE_H
