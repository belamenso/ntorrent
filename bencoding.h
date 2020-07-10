//
// Created by julian on 09.07.2020.
//

#ifndef NTORRENT_BENCODING_H
#define NTORRENT_BENCODING_H

#include <string>
#include <map>
#include <vector>
#include <optional>
#include <cassert>
#include <ostream>
#include <utility>
#include <sstream>
#include <memory>
#include <limits>


class BNode {
protected:
    static bool eat(const std::string& bdata, uint64_t& idx, char c) {
        if (bdata.size() <= idx) return false;
        return bdata[idx++] == c;
    }

    virtual void present(std::ostream& os) const = 0;

    BNode(uint64_t beg, uint64_t end) : beg(beg), end(end) {}

    BNode() : BNode(std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max()) {}

public:
    const uint64_t beg, end;

    static std::optional<std::pair<std::unique_ptr<BNode>, uint64_t>> decode(const std::string& bdata, uint64_t idx=0);

    [[nodiscard]] virtual std::string encode() const = 0;

    friend std::ostream& operator << (std::ostream& os, const BNode& bn) {
        bn.present(os);
        return os;
    }
};


class BInt: public BNode {
protected:
    void present(std::ostream& os) const override {
        os << value;
    }

public:
    const int64_t value = 0;

    explicit BInt(int64_t value) : value(value) {}

    BInt(int64_t value, uint64_t beg, uint64_t end) : BNode(beg, end), value(value) {}

    static std::optional<std::pair<std::unique_ptr<BInt>, uint64_t>> decode(const std::string& bdata, uint64_t idx=0);

    [[nodiscard]] std::string encode() const override {
        return "i" + std::to_string(value) + "e";
    }
};


class BString: public BNode {
protected:
    void present(std::ostream& os) const override {
        if (value.size() <= 50) {
            os << "“" << value << "”";
        } else {
            os << "<# string of length " << value.size() << " #>";
        }
    }

public:
    const std::string value;

    explicit BString(std::string value) : value(std::move(value)) {}

    BString(std::string value, uint64_t beg, uint64_t end) : BNode(beg, end), value(std::move(value)) {}

    [[nodiscard]] std::string encode() const override {
        return std::to_string(value.size()) + ":" + value;
    }

    static std::optional<std::pair<std::unique_ptr<BString>, uint64_t>> decode(const std::string& bdata, uint64_t idx=0);

    bool operator < (const BString& other) const {
        return value < other.value; // TODO is this based on binary encoding?
    }
};


class BList: public BNode {
protected:
    void present(std::ostream& os) const override {
        if (elements.empty()) {
            os << "[]";
        } else {
            os << "[";
            for (const auto& elp: elements) os << " " << *elp;
            os << " ]";
        }
    }

public:
    const std::vector<std::unique_ptr<BNode>> elements;

    explicit BList(std::vector<std::unique_ptr<BNode>> elements) : elements(std::move(elements)) {}

    BList(std::vector<std::unique_ptr<BNode>> elements, uint64_t beg, uint64_t end)
    : BNode(beg, end), elements(std::move(elements)) {}

    [[nodiscard]] std::string encode() const override {
        std::stringstream ret;
        ret << "l";
        for (const auto& el: elements)
            ret << el->encode();
        ret << "e";
        return ret.str();
    }

    static std::optional<std::pair<std::unique_ptr<BList>, uint64_t>> decode(const std::string& bdata, uint64_t idx=0);
};


class BDictionary: public BNode {
protected:
    void present(std::ostream& os) const override {
        if (dict.empty()) {
            os << "{}";
        } else {
            os << "{";
            for (const auto& [key, val]: dict) os << " " << key.value << ": " << *val << ";";
            os << " }";
        }
    }

public:
    const std::map<BString, std::unique_ptr<BNode>> dict;

    explicit BDictionary(std::map<BString, std::unique_ptr<BNode>> dict) : dict(std::move(dict)) {}

    BDictionary(std::map<BString, std::unique_ptr<BNode>> dict, uint64_t beg, uint64_t end)
    : BNode(beg, end), dict(std::move(dict)) {}

    static std::optional<std::pair<std::unique_ptr<BDictionary>, uint64_t>> decode(const std::string& bdata, uint64_t idx=0);

    [[nodiscard]] std::string encode() const override {
        std::stringstream ss;
        ss << "d";
        for (const auto& [k, v]: dict)
            ss << k.encode() << v->encode();
        ss << "e";
        return ss.str();
    }
};


std::optional<std::pair<std::unique_ptr<BInt>, uint64_t>> BInt::decode(const std::string& bdata, uint64_t idx) {
    const uint64_t beg = idx;
    if (!eat(bdata, idx, 'i')) return {};

    int64_t value = 0, sign = 1;
    if (bdata[idx] == '-') {
        sign = -1;
        idx++;
    }

    std::string number_as_seen;
    while (idx < bdata.size() and std::isdigit(bdata[idx])) {
        number_as_seen += bdata[idx];
        value = 10*value + (bdata[idx] - '0');
        idx++;
    }
    if (number_as_seen.size() >= 2 and number_as_seen[0] == 0) return {};
    if (idx >= bdata.size()) return {};

    if (!eat(bdata, idx, 'e')) return {};
    if (sign == -1 and value == 0) return {};
    return {{ std::make_unique<BInt>(value * sign, beg, idx), idx }};
}


std::optional<std::pair<std::unique_ptr<BString>, uint64_t>> BString::decode(const std::string& bdata, uint64_t idx) {
    const uint64_t beg = idx;
    if (bdata.size() <= idx) return {};
    uint64_t length = 0;
    if (!std::isdigit(bdata[idx])) return {};
    while (idx <= bdata.size() and std::isdigit(bdata[idx])) {
        length = 10*length + (bdata[idx++] - '0');
    }
    if (!eat(bdata, idx, ':')) return {};
    if (bdata.size() - idx < length) return {};
    return {{ std::make_unique<BString>(bdata.substr(idx, length), beg, idx + length), idx + length }};
}


std::optional<std::pair<std::unique_ptr<BList>, uint64_t>> BList::decode(const std::string& bdata, uint64_t idx) {
    const uint64_t beg = idx;
    if (bdata.size() <= idx) return {};
    if (!eat(bdata, idx, 'l')) return {};
    std::vector<std::unique_ptr<BNode>> elements;
    while (idx <= bdata.size() and bdata[idx] != 'e') {
        auto got = BNode::decode(bdata, idx);
        if (!got.has_value()) return {};
        elements.push_back(std::move(got.value().first));
        idx = got.value().second;
    }
    if (bdata.size() <= idx) return {};
    if (!eat(bdata, idx, 'e')) return {};
    return {{ std::make_unique<BList>(std::move(elements), beg, idx), idx }};
}


std::optional<std::pair<std::unique_ptr<BDictionary>, uint64_t>> BDictionary::decode(const std::string& bdata, uint64_t idx) {
    const uint64_t beg = idx;
    if (bdata.size() <= idx) return {};
    if (!eat(bdata, idx, 'd')) return {};
    std::map<BString, std::unique_ptr<BNode>> dict;
    while (idx < bdata.size() and bdata[idx] != 'e') {
        auto key = BString::decode(bdata, idx);
        if (!key.has_value()) return {};
        idx = key.value().second;
        auto val = BNode::decode(bdata, idx);
        if (!val.has_value()) return {};
        idx = val.value().second;
        if (dict.count(*key.value().first) >= 1) return {};
        dict[*key.value().first] = std::move(val.value().first);
    }
    if (bdata.size() <= idx) return {};
    if (!eat(bdata, idx, 'e')) return {};
    return {{ std::make_unique<BDictionary>(std::move(dict), beg, idx), idx }};
}


std::optional<std::pair<std::unique_ptr<BNode>, uint64_t>> BNode::decode(const std::string& bdata, uint64_t idx) {
    if (bdata.size() <= idx) return {};
    if (std::isdigit(bdata[idx])) return BString::decode(bdata, idx);
    switch (bdata[idx]) {
        case 'i': return BInt::decode(bdata, idx);
        case 'l': return BList::decode(bdata, idx);
        case 'd': return BDictionary::decode(bdata, idx);
        default:  return {};
    }
}


#endif //NTORRENT_BENCODING_H
