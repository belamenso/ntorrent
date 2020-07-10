//
// Created by julian on 10.07.2020.
//

#define CATCH_CONFIG_MAIN
#include "../lib/catch.hpp"

#include "../src/tracker_request.h"


TEST_CASE( "Validity of URL transforms", "[scrape_url_tests]" ) {
    REQUIRE( scrape_url("http://example.com/announce") == optional<string>("http://example.com/scrape") );
    REQUIRE( scrape_url("http://example.com/x/announce") == optional<string>("http://example.com/x/scrape") );
    REQUIRE( scrape_url("http://example.com/announce.php") == optional<string>("http://example.com/scrape.php") );
    REQUIRE( scrape_url("http://example.com/a") == optional<string>() );
    REQUIRE( scrape_url("http://example.com/announce?x2%0644") == optional<string>("http://example.com/scrape?x2%0644") );
    REQUIRE( scrape_url("http://example.com/announce?x=2/4") == optional<string>() );
    REQUIRE( scrape_url("http://example.com/x%064announce") == optional<string>() );
}


TEST_CASE( "Appending info_hashes", "[scrape_url_tests]" ) {
    REQUIRE( scrape_url("http://example.com/announce.php", {}) == optional<string>("http://example.com/scrape.php") );
    REQUIRE( scrape_url("http://example.com/announce.php", {"aaaaaaaaaaaaaaaaaaaa"})
        == optional<string>("http://example.com/scrape.php?info_hash=aaaaaaaaaaaaaaaaaaaa") );
    REQUIRE( scrape_url("http://example.com/announce.php", {"aaaaaaaaaaaaaaaaaaaa", "bbbbbbbbbbbbbbbbbbbb"})
        == optional<string>("http://example.com/scrape.php?info_hash=aaaaaaaaaaaaaaaaaaaa&info_hash=bbbbbbbbbbbbbbbbbbbb") );
    REQUIRE( scrape_url("http://example.com/announce.php", {"aaaaaaaaaaaaaaaaaaaa", "bbbbbbbbbbbbbbbbbbbb", "cccccccccccccccccccc"})
        == optional<string>("http://example.com/scrape.php?info_hash=aaaaaaaaaaaaaaaaaaaa&info_hash=bbbbbbbbbbbbbbbbbbbb&info_hash=cccccccccccccccccccc") );
}
