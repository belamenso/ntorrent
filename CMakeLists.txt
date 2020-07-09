cmake_minimum_required(VERSION 3.16)
project(ntorrent)

set(CMAKE_CXX_STANDARD 17)

add_executable(ntorrent main.cpp bencoding.cpp bencoding.h MetainfoFile.cpp MetainfoFile.h)