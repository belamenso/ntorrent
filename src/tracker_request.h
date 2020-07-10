//
// Created by julian on 10.07.2020.
//

#ifndef NTORRENT_TRACKER_REQUEST_H
#define NTORRENT_TRACKER_REQUEST_H

#include <string>
#include <ostream>
#include <cassert>
#include <optional>
using std::string, std::optional, std::vector;

#include "url_encoding.h"


enum event_t {
    started, stopped, completed
};


std::ostream& operator << (std::ostream& os, const event_t e) {
    if      (e == started)   os << "started";
    else if (e == stopped)   os << "stopped";
    else if (e == completed) os << "completed";
    else assert(false);
    return os;
}


struct tracker_request {
    const string announce;

    const string info_hash_urlencoded;
    const string peer_id_urlencoded;
    const uint16_t port;
    const uint64_t uploaded, downloaded, left;
    optional<bool> compact; // XXX optional since some trackers behave differently in "commpact=0" vs no compact at all
    optional<bool> no_peer_id;
    optional<event_t> event;
    optional<string> ip;
    optional<uint64_t> numwant;
    optional<string> key_urlencoded;
    optional<string> trackerid_urlencoded;

    tracker_request(string info_hash_urlencoded, string peer_id_urlencoded, uint16_t port,
            uint64_t uploaded, uint64_t downloaded, uint64_t left)
    : info_hash_urlencoded(std::move(info_hash_urlencoded)), peer_id_urlencoded(std::move(peer_id_urlencoded)),
    port(port), uploaded(uploaded), downloaded(downloaded), left(left) {}

    static tracker_request build(string info_hash_urlencoded, string peer_id_urlencoded, uint16_t port,
                                 uint64_t uploaded, uint64_t downloaded, uint64_t left) {
        return tracker_request(std::move(info_hash_urlencoded), std::move(peer_id_urlencoded), port,
                uploaded, downloaded, left);
    }

    tracker_request& set_compact(optional<bool> compact_) {
        compact = compact_;
        return *this;
    }

    tracker_request& set_no_peer_id(optional<bool> no_peer_id_) {
        no_peer_id = no_peer_id_;
        return *this;
    }

    tracker_request& set_event(optional<event_t> event_) {
        event = event_;
        return *this;
    }

    tracker_request& set_ip(optional<string> ip_) {
        ip = std::move(ip_);
        return *this;
    }

    tracker_request set_numwant(optional<uint64_t> numwant_) {
        numwant = numwant_;
        return *this;
    }

    tracker_request set_key_urlencoded(optional<string> key_urlencoded_) {
        key_urlencoded = std::move(key_urlencoded_);
        return *this;
    }

    tracker_request& set_trackerid(optional<string> trackerid_urlencoded_) {
        trackerid_urlencoded = std::move(trackerid_urlencoded_);
        return *this;
    }

    [[nodiscard]] string request() const {
        std::stringstream ss;
        ss << announce << "?"
           << "info_hash=" << info_hash_urlencoded
           << "&peer_id=" << peer_id_urlencoded
           << "&port=" << port
           << "&uploaded" << uploaded << "&downloaded" << downloaded << "&left" << left;
        if (compact.has_value()) ss << "&compact=" << (compact.value() ? "1" : "0");
        if (no_peer_id.has_value()) ss << "&no_peer_id=" << (no_peer_id.value() ? "1" : "0");
        if (event.has_value()) ss << "&event=" << event.value();
        if (ip.has_value()) ss << "&ip=" << ip.value();
        if (numwant.has_value()) ss << "&numwant=" << numwant.value();
        if (key_urlencoded.has_value()) ss << "&key=" << key_urlencoded.value();
        if (trackerid_urlencoded.has_value()) ss << "&trackerid=" << trackerid_urlencoded.value();

        return ss.str();
    }

};

optional<string> scrape_url(const string& announce, vector<string> info_hashes_urlencoded) {
    const auto announce_len = string("announce").length();

    int64_t i = announce.size() - 1;
    for (; i >= 0; i--)
        if (announce[i] == '/') break;
    if (i == -1) return {};
    i++; // eat the '/'
    if (announce.size() - i < announce_len) return {};
    if (string("announce") != announce.substr(i, announce_len)) return {};

    string ret = announce.substr(0, i) + "scrape" + announce.substr(i + announce_len, announce.size() - i);

    if (info_hashes_urlencoded.size()) {
        ret += "?";
        for (const string& ih: info_hashes_urlencoded)
            ret += "info_hash=" + ih + "&";
        ret.pop_back();
    }

    return { ret };
}

optional<string> scrape_url(const string& announce) {
    return scrape_url(announce, {});
}

#endif //NTORRENT_TRACKER_REQUEST_H