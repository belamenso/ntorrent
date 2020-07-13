//
// Created by julian on 13.07.2020.
//

#ifndef NTORRENT_BITVECTOR_H
#define NTORRENT_BITVECTOR_H

#include <vector>
#include <cstdint>
using std::vector;

class bitvector {
    uint64_t bits_;
    uint64_t ones_;
    vector<uint8_t> data_;

public:
    explicit bitvector(uint64_t bits_) : bits_(bits_), ones_(0), data_((bits_+7)/8, 0) {}

    [[nodiscard]] uint64_t size() const { return data_.size(); }

    [[nodiscard]] uint64_t bits() const { return bits_; }

    [[nodiscard]] uint64_t ones() const { return ones_; }

    [[nodiscard]] const uint8_t* data() const { return data_.data(); }

    void set_data(const uint8_t* new_data) {
        for (unsigned i = 0; i < data_.size(); i++)
            data_[i] = new_data[i];
    }

    bool set(unsigned i, bool bit) {
        auto h = i/8, l = 7-(i&7u);
        uint8_t lb = 1u<<l;
        assert( h < data_.size() );
        if (bool(data_[h] & lb) != bit) {
            if (bit) {
                data_[h] |= lb;
                ones_++;
            } else {
                data_[h] &= uint8_t(~lb);
                assert(ones_ >= 1);
                ones_--;
            }
        }
        return bit;
    }

    bool operator [] (unsigned i) const {
        auto h = i/8, l = 7 - (i&7u);
        uint8_t lb = 1u<<l;
        return data_[h] & lb;
    }
};

#endif //NTORRENT_BITVECTOR_H
