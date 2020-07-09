#include <iostream>
#include <fstream>
#include <cassert>

#include "bencoding.h"
#include "url_encoding.h"

int main() {
    using std::cout, std::endl;
    string name = "/home/julian/ntorrent/examples/cosmos-laundromat.torrent";

    std::ifstream example_file;
    example_file.open(name.c_str(), std::ios::binary);
    assert(example_file.is_open());
    std::stringstream buffer;
    buffer << example_file.rdbuf();

    auto got = BNode::decode(buffer.str());

    cout << *got.value().first << endl;

    cout << url_encode(name) << endl;
    cout << url_decode(url_encode(name)).value() << endl;
    assert( name == url_decode(url_encode(name)).value() );

    return 0;
}
