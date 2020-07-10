//
// Created by julian on 09.07.2020.
//

#ifndef NTORRENT_METAINFO_H
#define NTORRENT_METAINFO_H

#include <memory>
#include <string>
#include <vector>
#include <time.h>
#include <ostream>
#include <optional>
#include <stdexcept>

#include <iostream>
using std::cout, std::endl;

#include "bencoding.h"

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
             optional<vector<string>> announce_list, optional<uint64_t> creation_date, optional<string> comment,
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
    optional<vector<string>> announce_list;
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
           << "  announce: " << mi.announce << "\n"
           << ((not mi.creation_date.has_value()) ? "" : ("  creation date: " + string(date) + "\n"))
           << ((not mi.comment.has_value()) ? "" : ("  comment: " + mi.comment.value() + "\n"))
           << ((not mi.created_by.has_value()) ? "" : ("  created by: " + mi.created_by.value() + "\n"))
           << ((not mi.encoding.has_value()) ? "" : ("  encoding: " + mi.encoding.value() + "\n"))
           << "  files: [\n";
        for (const auto& el: mi.files) os << "    " << el << "\n";
        os << "  ]\n"
           << "}" << endl;
        return os;
    }


    static metainfo parse(std::unique_ptr<BNode> root) {
        const auto& dict = (dynamic_cast<BDictionary const *>(root.get()))->dict;

        uint64_t piece_length;
        string pieces;
        bool private_;
        string name;
        vector<file_description> files;
        string announce;
        optional<vector<string>> announce_list;
        optional<uint64_t> creation_date;
        optional<string> comment;
        optional<string> created_by;
        optional<string> encoding;

        if (0 == dict.count(BString("info"))) throw std::domain_error("No 'info' field.");
        const auto& info_dict = dynamic_cast<BDictionary*>(dict.at(BString("info")).get())->dict;

        if (0 == info_dict.count(BString("piece length"))) throw std::domain_error("No 'piece length' field.");
        piece_length = static_cast<uint64_t>(dynamic_cast<BInt*>(info_dict.at(BString("piece length")).get())->value);

        if (0 == info_dict.count(BString("pieces"))) throw std::domain_error("No 'pieces' field.");
        pieces = dynamic_cast<BString*>(info_dict.at(BString("pieces")).get())->value;
        if (pieces.length() % 20) throw std::domain_error("'pieces' field should have length dividable by 20.");

        if (0 == info_dict.count(BString("private"))) {
            private_ = false;
        } else {
            int64_t got = dynamic_cast<BInt*>(info_dict.at(BString("private")).get())->value;
            if (not (got == 0 or got == 1))
                throw std::domain_error("Field 'private' must be either 0 or 1, not " + std::to_string(got));
            private_ = got == 1;
        }

        if (0 == info_dict.count(BString("name"))) throw std::domain_error("No 'name' field.");
        name = dynamic_cast<BString*>(info_dict.at(BString("name")).get())->value;

        if (0 == dict.count(BString("announce"))) throw std::domain_error("No 'announce' field.");
        announce = dynamic_cast<BString*>(dict.at(BString("announce")).get())->value;

        if (1 <= dict.count(BString("announce list"))) {
            const auto& got_list = dynamic_cast<BList*>(dict.at(BString("announce list")).get())->elements;
            announce_list = { vector<string>() };
            for (const auto& el: got_list)
                announce_list.value().push_back(dynamic_cast<BString*>(el.get())->value);
        }

        if (1 == dict.count(BString("creation date")))
            creation_date = { static_cast<uint64_t>(
                                    dynamic_cast<BInt*>(dict.at(BString("creation date")).get())->value) };

        if (1 == dict.count(BString("comment")))
            comment = { dynamic_cast<BString*>(dict.at(BString("comment")).get())->value };

        if (1 == dict.count(BString("created by")))
            created_by = { dynamic_cast<BString*>(dict.at(BString("created by")).get())->value };

        if (1 == dict.count(BString("encoding")))
            encoding = { dynamic_cast<BString*>(dict.at(BString("encoding")).get())->value };

        if (1 == info_dict.count(BString("length"))) { /// XXX Single File Mode
            optional<string> md5sum;
            if (1 == info_dict.count(BString("md5sum"))) {
                md5sum = {dynamic_cast<BString *>(info_dict.at(BString("md5sum")).get())->value};
                if (not valid_md5sum_format(md5sum.value())) throw std::domain_error("Invalid md5 sum format.");
            }
            if (0 == info_dict.count(BString("length")))
                throw std::domain_error("No 'length' field in the single file mode.");
            uint64_t length = static_cast<uint64_t>(dynamic_cast<BInt*>(info_dict.at(BString("length")).get())->value);

            files.emplace_back( length, md5sum, vector<string>({name}) );
        } else { /// XXX Multiple Files Mode
            if (0 == info_dict.count(BString("files")))
                throw std::domain_error("No 'files' field in the multiple file mode.");
            const auto& files_field = dynamic_cast<BList*>(info_dict.at(BString("files")).get())->elements;
            for (const auto& file_dict_entry: files_field) {
                const auto& file_dict = dynamic_cast<BDictionary*>(file_dict_entry.get())->dict;

                uint64_t length;
                optional<string> md5sum;
                vector<string> path { name };

                if (0 == file_dict.count(BString("length")))
                    throw std::domain_error("No 'length' field in an entry of 'files' dictionary.");
                length = static_cast<uint64_t>(dynamic_cast<BInt*>(file_dict.at(BString("length")).get())->value);

                if (1 == file_dict.count(BString("md5sum"))) {
                    md5sum = {dynamic_cast<BString *>(file_dict.at(BString("md5sum")).get())->value};
                    if (not valid_md5sum_format(md5sum.value())) throw std::domain_error("Invalid md5 sum format.");
                }

                if (0 == file_dict.count(BString("path")))
                    throw std::domain_error("No 'path' field in an entry of 'files' dictionary.");
                for (const auto& path_el: dynamic_cast<BList*>(file_dict.at(BString("path")).get())->elements)
                    path.push_back( dynamic_cast<BString*>(path_el.get())->value );

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
