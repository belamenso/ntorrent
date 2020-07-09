//
// Created by julian on 09.07.2020.
//

#ifndef NTORRENT_METAINFOFILE_H
#define NTORRENT_METAINFOFILE_H

#include <string>
#include <optional>
#include <vector>
#include <memory>
using std::vector, std::string, std::optional;


struct InfoDictionary {
    const uint64_t piece_length;
    const string pieces;
    const bool private_;

protected:
    virtual void non_abstract_class() = 0;
};


struct SingleFileInfo : public InfoDictionary {
    const string name;
    const uint64_t length;
    const optional<string> md5sum;
private:
    void non_abstract_class() final {}
};


struct MultipleFileInfo : public InfoDictionary {
    struct file_description {
        const uint64_t length;
        const optional<string> md5sum;
        const vector<string> path;
    };

    const string name;
    const vector<file_description> files;
private:
    void non_abstract_class() final {}
};


class MetainfoFile {
public:
    const std::unique_ptr<InfoDictionary> info;
    const string announce;
    const optional<vector<string>> announce_list;
    const optional<uint64_t> creation_date;
    const optional<string> comment;
    const optional<string> created_by;
    const optional<string> encoding;
};


#endif //NTORRENT_METAINFOFILE_H
