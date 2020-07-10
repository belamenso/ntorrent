cmake_minimum_required(VERSION 3.16)
project(ntorrent)

set(CMAKE_CXX_STANDARD 17)

add_executable(ntorrent main.cpp
                src/bencoding.h src/metainfo.h src/url_encoding.h src/sha1.h
                src/tracker_response.h src/tracker_request.h)

add_subdirectory(tests)
