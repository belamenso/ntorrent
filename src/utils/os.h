//
// Created by julian on 12.07.2020.
//

#ifndef NTORRENT_OS_H
#define NTORRENT_OS_H

#include <unistd.h>
#include <iostream>


void write_all(const int fd, const char *data, unsigned n) {
    unsigned done = 0;
    while (done < n) {
        int got = write(fd, data + done, n);
        if (got == -1) {
            std::cerr << "??" << std::endl;
            return;
        }
        done += got;
    }
}

#endif //NTORRENT_OS_H
