#include <iostream>
#include <fstream>
#include <cassert>
#include <memory>

#include "src/bencoding.h"
#include "src/utils/url_encoding.h"
#include "src/tracker_response.h"
#include "src/metainfo.h"
#include "src/utils/sha1.h"
#include "src/utils/net.h"
#include "src/tracker_request.h"
#include "src/tracker_udp.h"

int main() {
    using std::cout, std::endl;

    string name = "/home/julian/ntorrent/examples/torrent/sintel.torrent";

    std::ifstream example_file;
    example_file.open(name.c_str(), std::ios::binary);
    assert(example_file.is_open());
    std::stringstream buffer;
    buffer << example_file.rdbuf();

    auto got = bnode::decode(buffer.str());
    assert(got.has_value());

    cout << *got.value().first << endl;

    /*
    /// url encodings
    cout << url_encode(name) << endl;
    cout << url_decode(url_encode(name)).value() << endl;
    assert( name == url_decode(url_encode(name)).value() );
    */

    cout << endl;

    /// scrape
    /*std::shared_ptr<bnode> shared = std::move(got.value().first);
    auto parsed = tracker_scrape::parse(shared);
    assert(parsed.has_value());
    cout << parsed.value() << endl;*/


    /// tracker responses
    /*std::shared_ptr<bnode> shared = std::move(got.value().first);
    auto parsed = tracker_response::parse(shared);
    assert(parsed.has_value());
    assert(std::holds_alternative<tracker_response>(parsed.value()));
    cout << std::get<tracker_response>(parsed.value()) << endl;*/


    /// torrent files
    std::shared_ptr<bnode> shared = std::move(got.value().first);
    auto parsed = metainfo::parse(shared);
    cout << parsed << endl;

    // cout << udp_scrape(parsed.announce_list.value()[2][0], { metainfo::info_hash(shared, true) }).value() << endl;
    cout << endl;
    cout <<
    udp_announce(tracker_request(
            parsed.announce_list.value()[0][0],
            metainfo::info_hash(shared, true),
            string("01234567890123456789"), 50'723,
            0, 0, 1252).set_numwant(2)).value()
    << endl;
    return 0;

    assert( metainfo::info_hash(buffer.str(), shared) == metainfo::info_hash(shared) );
    cout << metainfo::info_hash(shared) << endl;
    cout << url_encode_hash(metainfo::info_hash(shared)) << endl;

    auto got_data = http_request( scrape_url( parsed.announce ).value() );
    cout << got_data.value() << endl;
}
