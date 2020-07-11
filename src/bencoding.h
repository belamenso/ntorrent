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


enum bnode_type {
    bint_t, bstring_t, blist_t, bdictionary_t
};


class bnode {
protected:
    static bool eat(const std::string& bdata, uint64_t& idx, char c) {
        if (bdata.size() <= idx) return false;
        return bdata[idx++] == c;
    }

    virtual void present(std::ostream& os) const = 0;

    bnode(uint64_t beg, uint64_t end) : beg(beg), end(end) {}

    bnode() : bnode(std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max()) {}

public:
    const uint64_t beg, end;

    static std::optional<std::pair<std::unique_ptr<bnode>, uint64_t>> decode(const std::string& bdata, uint64_t idx=0);

    [[nodiscard]] virtual std::string encode() const = 0;

    friend std::ostream& operator << (std::ostream& os, const bnode& bn) {
        bn.present(os);
        return os;
    }

    [[nodiscard]] virtual bnode_type type() const = 0;
};


class bint: public bnode {
protected:
    void present(std::ostream& os) const override {
        os << value;
    }

public:
    const int64_t value = 0;

    explicit bint(int64_t value) : value(value) {}

    bint(int64_t value, uint64_t beg, uint64_t end) : bnode(beg, end), value(value) {}

    static std::optional<std::pair<std::unique_ptr<bint>, uint64_t>> decode(const std::string& bdata, uint64_t idx=0);

    [[nodiscard]] std::string encode() const override {
        return "i" + std::to_string(value) + "e";
    }

    [[nodiscard]] bnode_type type() const override { return bint_t; }
};


class bstring: public bnode {
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

    explicit bstring(std::string value) : value(std::move(value)) {}

    bstring(std::string value, uint64_t beg, uint64_t end) : bnode(beg, end), value(std::move(value)) {}

    [[nodiscard]] std::string encode() const override {
        return std::to_string(value.size()) + ":" + value;
    }

    [[nodiscard]] bnode_type type() const override { return bstring_t; }

    static std::optional<std::pair<std::unique_ptr<bstring>, uint64_t>> decode(const std::string& bdata, uint64_t idx=0);

    bool operator < (const bstring& other) const {
        return value < other.value; // TODO is this based on binary encoding?
    }
};


class blist: public bnode {
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
    const std::vector<std::unique_ptr<bnode>> elements;

    explicit blist(std::vector<std::unique_ptr<bnode>> elements) : elements(std::move(elements)) {}

    blist(std::vector<std::unique_ptr<bnode>> elements, uint64_t beg, uint64_t end)
    : bnode(beg, end), elements(std::move(elements)) {}

    [[nodiscard]] std::string encode() const override {
        std::stringstream ret;
        ret << "l";
        for (const auto& el: elements)
            ret << el->encode();
        ret << "e";
        return ret.str();
    }

    [[nodiscard]] bnode_type type() const override { return blist_t; }

    static std::optional<std::pair<std::unique_ptr<blist>, uint64_t>> decode(const std::string& bdata, uint64_t idx=0);
};


class bdictionary: public bnode {
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
    const std::map<bstring, std::unique_ptr<bnode>> dict;

    explicit bdictionary(std::map<bstring, std::unique_ptr<bnode>> dict) : dict(std::move(dict)) {}

    bdictionary(std::map<bstring, std::unique_ptr<bnode>> dict, uint64_t beg, uint64_t end)
    : bnode(beg, end), dict(std::move(dict)) {}

    [[nodiscard]] bool has(const std::string& key) const {
        return 1 <= dict.count(bstring(key));
    }

    [[nodiscard]] bool has(const std::string& key, bnode_type type) const {
        const auto k = bstring(key);
        return 1 <= dict.count(k) and type == dict.at(k)->type();
    }

    [[nodiscard]] std::optional<int64_t> get_int(const std::string& key) const {
        auto k = bstring(key);
        if (not has(key, bint_t)) return {};
        return { dynamic_cast<bint*>(dict.at(k).get())->value };
    }

    [[nodiscard]] std::optional<std::string> get_string(const std::string& key) const {
        auto k = bstring(key);
        if (not has(key, bstring_t)) return {};
        return { dynamic_cast<bstring*>(dict.at(k).get())->value };
    }

    static std::optional<std::pair<std::unique_ptr<bdictionary>, uint64_t>> decode(const std::string& bdata, uint64_t idx=0);

    [[nodiscard]] std::string encode() const override {
        std::stringstream ss;
        ss << "d";
        for (const auto& [k, v]: dict)
            ss << k.encode() << v->encode();
        ss << "e";
        return ss.str();
    }

    [[nodiscard]] bnode_type type() const override { return bdictionary_t; }
};


std::optional<std::pair<std::unique_ptr<bint>, uint64_t>> bint::decode(const std::string& bdata, uint64_t idx) {
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
    return {{ std::make_unique<bint>(value * sign, beg, idx), idx }};
}


std::optional<std::pair<std::unique_ptr<bstring>, uint64_t>> bstring::decode(const std::string& bdata, uint64_t idx) {
    const uint64_t beg = idx;
    if (bdata.size() <= idx) return {};
    uint64_t length = 0;
    if (!std::isdigit(bdata[idx])) return {};
    while (idx <= bdata.size() and std::isdigit(bdata[idx])) {
        length = 10*length + (bdata[idx++] - '0');
    }
    if (!eat(bdata, idx, ':')) return {};
    if (bdata.size() - idx < length) return {};
    return {{ std::make_unique<bstring>(bdata.substr(idx, length), beg, idx + length), idx + length }};
}


std::optional<std::pair<std::unique_ptr<blist>, uint64_t>> blist::decode(const std::string& bdata, uint64_t idx) {
    const uint64_t beg = idx;
    if (bdata.size() <= idx) return {};
    if (!eat(bdata, idx, 'l')) return {};
    std::vector<std::unique_ptr<bnode>> elements;
    while (idx <= bdata.size() and bdata[idx] != 'e') {
        auto got = bnode::decode(bdata, idx);
        if (!got.has_value()) return {};
        elements.push_back(std::move(got.value().first));
        idx = got.value().second;
    }
    if (bdata.size() <= idx) return {};
    if (!eat(bdata, idx, 'e')) return {};
    return {{ std::make_unique<blist>(std::move(elements), beg, idx), idx }};
}


std::optional<std::pair<std::unique_ptr<bdictionary>, uint64_t>> bdictionary::decode(const std::string& bdata, uint64_t idx) {
    const uint64_t beg = idx;
    if (bdata.size() <= idx) return {};
    if (!eat(bdata, idx, 'd')) return {};
    std::map<bstring, std::unique_ptr<bnode>> dict;
    while (idx < bdata.size() and bdata[idx] != 'e') {
        auto key = bstring::decode(bdata, idx);
        if (!key.has_value()) return {};
        idx = key.value().second;
        auto val = bnode::decode(bdata, idx);
        if (!val.has_value()) return {};
        idx = val.value().second;
        if (dict.count(*key.value().first) >= 1) return {};
        dict[*key.value().first] = std::move(val.value().first);
    }
    if (bdata.size() <= idx) return {};
    if (!eat(bdata, idx, 'e')) return {};
    return {{ std::make_unique<bdictionary>(std::move(dict), beg, idx), idx }};
}


std::optional<std::pair<std::unique_ptr<bnode>, uint64_t>> bnode::decode(const std::string& bdata, uint64_t idx) {
    if (bdata.size() <= idx) return {};
    if (std::isdigit(bdata[idx])) return bstring::decode(bdata, idx);
    switch (bdata[idx]) {
        case 'i': return bint::decode(bdata, idx);
        case 'l': return blist::decode(bdata, idx);
        case 'd': return bdictionary::decode(bdata, idx);
        default:  return {};
    }
}


#endif //NTORRENT_BENCODING_H
