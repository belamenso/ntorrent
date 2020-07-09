#include <iostream>
#include <fstream>
#include <cassert>

#include "bencoding.h"

int main() {
    using std::cout, std::endl;

    // assert(false);
    std::ifstream example_file;
    example_file.open("/home/julian/ntorrent/example.torrent", std::ios::binary);
    assert(example_file.is_open());
    std::stringstream buffer;
    buffer << example_file.rdbuf();

    auto got = BNode::decode(buffer.str());
    cout << got.has_value() << endl;

    cout << *got.value().first << endl;

    return 0;
}
