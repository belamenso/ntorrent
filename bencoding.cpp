//
// Created by julian on 09.07.2020.
//

#include "bencoding.h"

std::optional<std::pair<std::unique_ptr<BInt>, uint64_t>> BInt::decode(const std::string& bdata, uint64_t idx) {
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
    return {{ std::make_unique<BInt>(value * sign), idx }};
}


std::optional<std::pair<std::unique_ptr<BString>, uint64_t>> BString::decode(const std::string& bdata, uint64_t idx) {
    if (bdata.size() <= idx) return {};
    uint64_t length = 0;
    if (!std::isdigit(bdata[idx])) return {};
    while (idx <= bdata.size() and std::isdigit(bdata[idx])) {
        length = 10*length + (bdata[idx++] - '0');
    }
    if (!eat(bdata, idx, ':')) return {};
    if (bdata.size() - idx < length) return {};
    return {{ std::make_unique<BString>(bdata.substr(idx, length)), idx + length }};
}


std::optional<std::pair<std::unique_ptr<BList>, uint64_t>> BList::decode(const std::string& bdata, uint64_t idx) {
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
    return {{ std::make_unique<BList>(std::move(elements)), idx }};
}


std::optional<std::pair<std::unique_ptr<BDictionary>, uint64_t>> BDictionary::decode(const std::string& bdata, uint64_t idx) {
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
    return {{ std::make_unique<BDictionary>(std::move(dict)), idx }};
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
