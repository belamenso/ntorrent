//
// Created by julian on 11.07.2020.
//

#ifndef NTORRENT_HTTP_REQUEST_H
#define NTORRENT_HTTP_REQUEST_H

#include <string>
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <curl/curl.h>

using std::string;


static size_t write_function(const char* data, size_t size, size_t actual_size, void *str_ptr) {
    string& ret = *static_cast<string*>(str_ptr);
    assert(size == 1);
    ret += string(data, actual_size);
    return actual_size;
}


string http_request(const string& url) {
    if (not (url.substr(0, string("http://").length()) == "http://"
                or url.substr(0, string("https://").length()) == "https://"))
        throw std::domain_error("Incorrect protocol. Only HTTP and HTTPS are supported");

    CURL *curl;
    CURLcode res;
    string ret;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if (not curl) {

    } else {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_function);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, static_cast<void*>(&ret));
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }
    }
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    return ret;
}

#endif //NTORRENT_HTTP_REQUEST_H
