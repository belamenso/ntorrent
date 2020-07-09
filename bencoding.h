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

class BNode {
protected:
    static bool eat(const std::string& bdata, uint64_t& idx, char c) {
        if (bdata.size() <= idx) return false;
        return bdata[idx++] == c;
    }

    virtual void present(std::ostream& os) const = 0;

public:
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

    static std::optional<std::pair<std::unique_ptr<BInt>, uint64_t>> decode(const std::string& bdata, uint64_t idx=0);

    [[nodiscard]] std::string encode() const override {
        return "i" + std::to_string(value) + "e";
    }
};

class BString: public BNode {
protected:
    void present(std::ostream& os) const override {
        os << "“" << value << "”";
    }

public:
    const std::string value;

    explicit BString(std::string value) : value(std::move(value)) {}

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
            for (const auto& [key, val]: dict) os << " " << key.value << ": " << *val;
            os << " }";
        }
    }

public:
    const std::map<BString, std::unique_ptr<BNode>> dict;

    explicit BDictionary(std::map<BString, std::unique_ptr<BNode>> dict) : dict(std::move(dict)) {}

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

#endif //NTORRENT_BENCODING_H
