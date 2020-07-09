#include <iostream>
#include <fstream>
#include <cassert>

#include "bencoding.h"
#include "url_encoding.h"

int main() {
    using std::cout, std::endl;

    std::ifstream example_file;
    example_file.open("/home/julian/ntorrent/example.torrent", std::ios::binary);
    assert(example_file.is_open());
    std::stringstream buffer;
    buffer << example_file.rdbuf();

    auto got = BNode::decode(buffer.str());
    cout << got.has_value() << endl;

    cout << *got.value().first << endl;

    cout << url_encode("ałajć~") << endl;
    cout << url_decode(url_encode("ałajć~")).value() << endl;

    return 0;
}
