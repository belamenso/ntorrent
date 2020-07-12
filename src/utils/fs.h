//
// Created by julian on 12.07.2020.
//

#ifndef NTORRENT_FS_H
#define NTORRENT_FS_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <optional>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <boost/filesystem.hpp>
using std::string, std::optional, std::vector;
using namespace boost::filesystem;

#include "../metainfo.h"


void release_memory_mappings(const vector<std::pair<uint8_t*, uint64_t>>& addresses) {
    for (auto& [addr, size]: addresses)
        munmap(addr, size);
}

optional<vector<std::pair<uint8_t*, uint64_t>>> get_memory_mapped_files(const path& root, const vector<file_description>& files) {
    // TODO races obviously
    boost::system::error_code err;

    if (exists(root)) {
        if (not is_directory(root)) return {};
    } else {
        err.clear();
        create_directories(root, err);
        if (err) return {};
    }

    vector<path> paths;
    for (const auto& file_desc: files) {
        path file_rel_path;
        for (const string& part: file_desc.path) file_rel_path /= path(part);
        if (not file_rel_path.is_relative()) return {};
        paths.push_back( root / file_rel_path );
    }

    for (unsigned i = 0; i < files.size(); i++) {
        err.clear();
        create_directories(paths[i].parent_path(), err);
        if (err) return {};

        if (exists(paths[i])) {
            if (is_regular_file(paths[i])) {
                if (file_size(paths[i]) != files[i].length) return {};
            } else return {};
        } else {
            std::ofstream ofs(paths[i].c_str(), std::ios::binary | std::ios::out);
            if (files[i].length < 1) return {};
            ofs.seekp(files[i].length - 1);
            ofs.write("", 1);
        }
    }

    vector<std::pair<uint8_t*, uint64_t>> addresses;
    for (unsigned i = 0; i < files.size(); i++) {
        int fd = open(paths[i].c_str(), O_RDWR);
        if (fd == -1) goto mmap_failure;
        void* got = mmap(nullptr, files[i].length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (got == MAP_FAILED) goto mmap_failure;
        addresses.emplace_back( static_cast<uint8_t*>(got), files[i].length );
        close(fd);
    }

    assert(addresses.size() == files.size());
    return { addresses };

mmap_failure:
    release_memory_mappings(addresses);
    return {};
}

#endif //NTORRENT_FS_H
