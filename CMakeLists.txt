cmake_minimum_required(VERSION 3.16)
project(ntorrent)

set(CMAKE_CXX_STANDARD 17)

link_libraries(curl)

add_executable(ntorrent main.cpp
                src/bencoding.h src/metainfo.h src/utils/url_encoding.h src/utils/sha1.h
                src/tracker_response.h src/tracker_request.h src/utils/encoding.h src/utils/net.h src/tracker_udp.h src/utils/os.h)

add_subdirectory(tests)
