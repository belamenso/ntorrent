# ntorrent - torrent client library

Header-only C++17 Linux implementation of various bittorrent-related
protocols. Mostly for educational purposes.

### goals
1. correctness of implementation
1. usability both as a standalone client and library
1. clarity of code
1. POSIX portability

### non-goals
- performance
- full portability

### done
- [x] bencoding parser
- [x] metainfo parser
- [x] TCP tracker protocol
    - [x] Tracker Returns Compact Peer Lists ([BEP_23](http://bittorrent.org/beps/bep_0023.html))
- [ ] TCP tracker protocol ([BEP_15](http://bittorrent.org/beps/bep_0015.html))
    - [x] IPv4
    - [ ] IPv6
- [ ] main client
- [ ] DHT + magnet links support
- [ ] Python3 module

### dependencies
- [libcurl](https://curl.haxx.se/libcurl/) (easy to replace, portable)
- [boost::filesystem](https://www.boost.org/doc/libs/1_73_0/libs/filesystem/doc/index.htm) (easy to replace, will be replaced)
- [neither](https://github.com/LoopPerfect/neither) (header-only, portable)
- [Catch2](https://github.com/catchorg/Catch2) (header-only, only for tests)
