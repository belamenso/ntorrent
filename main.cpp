#include <iostream>
#include <fstream>
#include <cassert>
#include <memory>

#include "src/bencoding.h"
#include "src/url_encoding.h"
#include "src/tracker_response.h"
#include "src/metainfo.h"
#include "src/sha1.h"

int main() {
    using std::cout, std::endl;

    string name = "/home/julian/ntorrent/examples/response-compact";

    std::ifstream example_file;
    example_file.open(name.c_str(), std::ios::binary);
    assert(example_file.is_open());
    std::stringstream buffer;
    buffer << example_file.rdbuf();

    auto got = BNode::decode(buffer.str());
    assert(got.has_value());

    cout << *got.value().first << endl;

    cout << url_encode(name) << endl;
    cout << url_decode(url_encode(name)).value() << endl;
    assert( name == url_decode(url_encode(name)).value() );

    cout << endl;

    std::shared_ptr<BNode> shared = std::move(got.value().first);
    auto parsed = tracker_response::parse(shared);
    assert(parsed.has_value());
    assert(std::holds_alternative<tracker_response>(parsed.value()));
    cout << std::get<tracker_response>(parsed.value()) << endl;

    /*std::shared_ptr<BNode> shared = std::move(got.value().first);
    auto parsed = metainfo::parse(shared);
    cout << parsed << endl;

    assert( metainfo::info_hash(buffer.str(), shared) == metainfo::info_hash(shared) );
    cout << metainfo::info_hash(shared) << endl;
    cout << url_encode_hash(metainfo::info_hash(shared)) << endl;*/
}
