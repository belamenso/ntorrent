cmake_minimum_required(VERSION 3.16)
project(ntorrent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)

link_libraries(curl)

find_package(Boost 1.67 COMPONENTS system filesystem REQUIRED)

add_executable(ntorrent main.cpp
                src/bencoding.h src/metainfo.h src/utils/url_encoding.h src/utils/sha1.h
                src/tracker_response.h src/tracker_request.h src/utils/encoding.h src/utils/net.h
                src/tracker_udp.h src/utils/os.h src/utils/fs.h)
target_link_libraries(ntorrent ${Boost_FILESYSTEM_LIBRARY} ${Boost_SYSTEM_LIBRARY})

add_subdirectory(tests)
