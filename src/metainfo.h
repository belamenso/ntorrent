//
// Created by julian on 09.07.2020.
//

#ifndef NTORRENT_METAINFO_H
#define NTORRENT_METAINFO_H

#include <ctime>
#include <memory>
#include <string>
#include <vector>
#include <limits>
#include <ostream>
#include <optional>
#include <stdexcept>

#include <iostream>
using std::cout, std::endl;

#include "bencoding.h"
#include "sha1.h"

using std::vector, std::string, std::optional;


static inline bool valid_md5sum_format(const string& sum) {
    if (sum.length() != 32) return false;
    for (uint8_t c: sum)
        if (not (('0' <= c and c <= '9') or ('a' <= c and c <= 'f') or ('A' <= c and c <= 'F'))) return false;
    return true;
}


struct file_description {
    uint64_t length;
    optional<string> md5sum;
    vector<string> path; // "name" field is now the first element in all paths now

    file_description(uint64_t length, optional<string> md5sum, vector<string> path)
    : length(length), md5sum(std::move(md5sum)), path(std::move(path)) {}

    friend std::ostream& operator << (std::ostream& os, const file_description& fd) {
        os << "File[len: " << fd.length << ", path: *";
        for (const auto& el: fd.path) os << "/" << el;
        if (fd.md5sum.has_value()) {
            os << ", md5sum: " << fd.md5sum.value() << "]";
        } else {
            os << "]";
        }
        return os;
    }
};


class metainfo {
protected:
    metainfo(uint64_t piece_length, string pieces, vector<file_description> files, bool private_, string announce,
             optional<vector<vector<string>>> announce_list, optional<uint64_t> creation_date, optional<string> comment,
             optional<string> created_by, optional<string> encoding)
    : piece_length(piece_length), pieces(std::move(pieces)), files(std::move(files)), private_(private_),
    announce(std::move(announce)), announce_list(std::move(announce_list)), creation_date(creation_date),
    comment(std::move(comment)), created_by(std::move(created_by)), encoding(std::move(encoding)) {}

public:
    uint64_t piece_length;
    string pieces;
    vector<file_description> files;
    bool private_ = false;
    string announce;
    optional<vector<vector<string>>> announce_list;
    optional<uint64_t> creation_date;
    optional<string> comment;
    optional<string> created_by;
    optional<string> encoding;


    friend std::ostream& operator << (std::ostream& os, const metainfo& mi) {
        char date[30];
        if (mi.creation_date.has_value()) {
            // mi.creation_date.value()
            time_t t = mi.creation_date.value();
            tm *tm = localtime(&t);
            strftime(date, sizeof(date)/sizeof(char), "%d.%m.%Y %T", tm);
        }

        os << "{\n"
           << "  piece length: " << mi.piece_length << "\n"
           << "  pieces: " << (mi.pieces.size() >= 50 ?
                                ("<<string of length " + std::to_string(mi.pieces.size()) + ">>") : mi.pieces) << "\n"
           << "  private: " << mi.private_ << "\n"
           << "  announce: " << mi.announce << "\n";
        if (mi.announce_list.has_value()) {
            os << "  announce list: [\n";
            for (const vector<string>& els: mi.announce_list.value()) {
                os << "    [";
                for (const string& el: els)
                    os << " " << el;
                os << " ]\n";
            }
            os << "  ]\n";
        }
        os << ((not mi.creation_date.has_value()) ? "" : ("  creation date: " + string(date) + "\n"))
           << ((not mi.comment.has_value()) ? "" : ("  comment: " + mi.comment.value() + "\n"))
           << ((not mi.created_by.has_value()) ? "" : ("  created by: " + mi.created_by.value() + "\n"))
           << ((not mi.encoding.has_value()) ? "" : ("  encoding: " + mi.encoding.value() + "\n"))
           << "  files: [\n";
        for (const auto& el: mi.files) os << "    " << el << "\n";
        os << "  ]\n"
           << "}" << endl;
        return os;
    }


    static string info_hash(const std::shared_ptr<bnode>& root) {
        const auto& root_dict = dynamic_cast<bdictionary*>(root.get())->dict;
        if (0 == root_dict.count(bstring("info"))) throw std::domain_error("No 'info' field.");
        auto info = dynamic_cast<bdictionary*>(root_dict.at(bstring("info")).get());
        return sha1sum(info->encode());
    }


    static string info_hash(const string& data, const std::shared_ptr<bnode>& root) {
        const auto& root_dict = dynamic_cast<bdictionary*>(root.get())->dict;
        if (0 == root_dict.count(bstring("info"))) throw std::domain_error("No 'info' field.");
        auto info = dynamic_cast<bdictionary*>(root_dict.at(bstring("info")).get());
        if (data.size() <= info->beg or data.size()+1 <= info->end)
            throw std::domain_error("Incorrect original slice encoding in the 'info' dictionary.");
        return sha1sum(data.substr(info->beg, info->end - info->beg));
    }


    static metainfo parse(const std::shared_ptr<bnode>& root) {
        const auto& dict = (dynamic_cast<bdictionary const *>(root.get()))->dict;

        uint64_t piece_length;
        string pieces;
        bool private_;
        string name;
        vector<file_description> files;
        string announce;
        optional<vector<vector<string>>> announce_list;
        optional<uint64_t> creation_date;
        optional<string> comment;
        optional<string> created_by;
        optional<string> encoding;

        if (0 == dict.count(bstring("info"))) throw std::domain_error("No 'info' field.");
        const auto& info_dict = dynamic_cast<bdictionary*>(dict.at(bstring("info")).get())->dict;

        if (0 == info_dict.count(bstring("piece length"))) throw std::domain_error("No 'piece length' field.");
        piece_length = static_cast<uint64_t>(dynamic_cast<bint*>(info_dict.at(bstring("piece length")).get())->value);

        if (0 == info_dict.count(bstring("pieces"))) throw std::domain_error("No 'pieces' field.");
        pieces = dynamic_cast<bstring*>(info_dict.at(bstring("pieces")).get())->value;
        if (pieces.length() % 20) throw std::domain_error("'pieces' field should have length dividable by 20.");

        if (0 == info_dict.count(bstring("private"))) {
            private_ = false;
        } else {
            int64_t got = dynamic_cast<bint*>(info_dict.at(bstring("private")).get())->value;
            if (not (got == 0 or got == 1))
                throw std::domain_error("Field 'private' must be either 0 or 1, not " + std::to_string(got));
            private_ = got == 1;
        }

        if (0 == info_dict.count(bstring("name"))) throw std::domain_error("No 'name' field.");
        name = dynamic_cast<bstring*>(info_dict.at(bstring("name")).get())->value;

        if (0 == dict.count(bstring("announce"))) throw std::domain_error("No 'announce' field.");
        announce = dynamic_cast<bstring*>(dict.at(bstring("announce")).get())->value;

        if (1 <= dict.count(bstring("announce-list"))) {
            const auto& got_list = dynamic_cast<blist*>(dict.at(bstring("announce-list")).get())->elements;
            announce_list = { vector<vector<string>>() };
            for (const auto& inner_list: got_list) {
                announce_list.value().emplace_back();
                for (const auto& str: dynamic_cast<blist*>(inner_list.get())->elements)
                    announce_list.value().back().push_back( dynamic_cast<bstring*>(str.get())->value );
            }
        }

        if (1 == dict.count(bstring("creation date")))
            creation_date = { static_cast<uint64_t>(
                                    dynamic_cast<bint*>(dict.at(bstring("creation date")).get())->value) };

        if (1 == dict.count(bstring("comment")))
            comment = { dynamic_cast<bstring*>(dict.at(bstring("comment")).get())->value };

        if (1 == dict.count(bstring("created by")))
            created_by = { dynamic_cast<bstring*>(dict.at(bstring("created by")).get())->value };

        if (1 == dict.count(bstring("encoding")))
            encoding = { dynamic_cast<bstring*>(dict.at(bstring("encoding")).get())->value };

        if (1 == info_dict.count(bstring("length"))) { /// XXX Single File Mode
            optional<string> md5sum;
            if (1 == info_dict.count(bstring("md5sum"))) {
                md5sum = {dynamic_cast<bstring *>(info_dict.at(bstring("md5sum")).get())->value};
                if (not valid_md5sum_format(md5sum.value())) throw std::domain_error("Invalid md5 sum format.");
            }
            if (0 == info_dict.count(bstring("length")))
                throw std::domain_error("No 'length' field in the single file mode.");
            uint64_t length = static_cast<uint64_t>(dynamic_cast<bint*>(info_dict.at(bstring("length")).get())->value);

            files.emplace_back( length, md5sum, vector<string>({name}) );
        } else { /// XXX Multiple Files Mode
            if (0 == info_dict.count(bstring("files")))
                throw std::domain_error("No 'files' field in the multiple file mode.");
            const auto& files_field = dynamic_cast<blist*>(info_dict.at(bstring("files")).get())->elements;
            for (const auto& file_dict_entry: files_field) {
                const auto& file_dict = dynamic_cast<bdictionary*>(file_dict_entry.get())->dict;

                uint64_t length;
                optional<string> md5sum;
                vector<string> path { name };

                if (0 == file_dict.count(bstring("length")))
                    throw std::domain_error("No 'length' field in an entry of 'files' dictionary.");
                length = static_cast<uint64_t>(dynamic_cast<bint*>(file_dict.at(bstring("length")).get())->value);

                if (1 == file_dict.count(bstring("md5sum"))) {
                    md5sum = {dynamic_cast<bstring *>(file_dict.at(bstring("md5sum")).get())->value};
                    if (not valid_md5sum_format(md5sum.value())) throw std::domain_error("Invalid md5 sum format.");
                }

                if (0 == file_dict.count(bstring("path")))
                    throw std::domain_error("No 'path' field in an entry of 'files' dictionary.");
                for (const auto& path_el: dynamic_cast<blist*>(file_dict.at(bstring("path")).get())->elements)
                    path.push_back( dynamic_cast<bstring*>(path_el.get())->value );

                files.emplace_back( length, md5sum, path );
            }
        }

        return metainfo(
                piece_length,
                pieces,
                files,
                private_,
                announce,
                announce_list,
                creation_date,
                comment,
                created_by,
                encoding);
    }
};


#endif //NTORRENT_METAINFO_H