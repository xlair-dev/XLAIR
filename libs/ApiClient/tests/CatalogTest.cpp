#include <ThirdParty/Catch2/catch.hpp>

#include "Parser.hpp"

namespace api = xlair::api;

namespace {
    s3d::JSON Catalog() {
        return s3d::JSON::Parse(UR"([
            {
                "music": {
                    "id": "music-1", "title": "Test", "artist": "Artist", "bpm": 120,
                    "genre": "ORIGINAL", "jacket": null, "audio": null,
                    "registrationDate": "2026-09-14T00:00:00Z", "isTest": true
                },
                "sheets": [
                    {"id":"sheet-1", "musicId":"music-1", "difficulty":"basic",
                     "level":3, "notesDesigner":"Designer", "chart":null},
                    {"id":"sheet-2", "musicId":"music-1", "difficulty":"advanced",
                     "level":7, "notesDesigner":"Designer", "chart":null},
                    {"id":"sheet-3", "musicId":"music-1", "difficulty":"master",
                     "level":12.5, "notesDesigner":"Designer", "chart":null}
                ]
            }
        ])");
    }
}

TEST_CASE("Catalog parses basic, advanced and master sheets with nullable assets", "[API][Catalog]") {
    const auto catalog = api::ParseCatalog(Catalog());
    REQUIRE(catalog.size() == 1);
    REQUIRE(catalog[0].sheets.size() == 3);
    CHECK(catalog[0].sheets[0].difficulty == U"basic");
    CHECK(catalog[0].sheets[1].difficulty == U"advanced");
    CHECK(catalog[0].sheets[2].difficulty == U"master");
    CHECK_FALSE(catalog[0].jacket.has_value());
    CHECK_FALSE(catalog[0].audio.has_value());
    CHECK_FALSE(catalog[0].sheets[0].chart.has_value());
}

TEST_CASE("Catalog preserves asset URLs and update timestamps", "[API][Catalog]") {
    auto json = Catalog();
    const auto asset = [](s3d::StringView url) {
        s3d::JSON value;
        value[U"url"] = s3d::String{ url };
        value[U"updatedAt"] = U"2026-09-14T00:00:00+00:00";
        return value;
    };
    json[0][U"music"][U"jacket"] = asset(U"/musics/music-1/jacket/hash.png");
    json[0][U"music"][U"audio"] = asset(U"/musics/music-1/audio/hash.wav");
    json[0][U"sheets"][0][U"chart"] = asset(U"/sheets/sheet-1/chart/hash.sus");
    const auto catalog = api::ParseCatalog(json);
    REQUIRE(catalog[0].jacket.has_value());
    REQUIRE(catalog[0].audio.has_value());
    REQUIRE(catalog[0].sheets[0].chart.has_value());
    CHECK(catalog[0].jacket->url == U"/musics/music-1/jacket/hash.png");
    CHECK(catalog[0].audio->url == U"/musics/music-1/audio/hash.wav");
    CHECK(catalog[0].sheets[0].chart->url == U"/sheets/sheet-1/chart/hash.sus");
    CHECK(catalog[0].jacket->updated_at == U"2026-09-14T00:00:00+00:00");
    CHECK(catalog[0].audio->updated_at == U"2026-09-14T00:00:00+00:00");
    CHECK(catalog[0].sheets[0].chart->updated_at == U"2026-09-14T00:00:00+00:00");
}

TEST_CASE("Catalog accepts omitted assets", "[API][Catalog]") {
    auto json = s3d::JSON::Parse(UR"([{
        "music":{"id":"m", "title":"T", "artist":"A", "bpm":120,
                 "genre":"OTHER", "registrationDate":"2026-09-14T00:00:00Z", "isTest":false},
        "sheets":[{"id":"s", "musicId":"m", "difficulty":"basic", "level":1, "notesDesigner":"D"}]
    }])");
    const auto catalog = api::ParseCatalog(json);
    CHECK_FALSE(catalog[0].jacket.has_value());
    CHECK_FALSE(catalog[0].audio.has_value());
    CHECK_FALSE(catalog[0].sheets[0].chart.has_value());
}

TEST_CASE("Assets require both URL and update timestamp strings", "[API][Catalog]") {
    for (const auto* source : { UR"("/asset.png")",
                                UR"({})",
                                UR"({"url":"/asset"})",
                                UR"({"updatedAt":"2026-09-14T00:00:00Z"})",
                                UR"({"url":42,"updatedAt":"2026-09-14T00:00:00Z"})",
                                UR"({"url":"/asset","updatedAt":null})",
                                UR"({"url":"/asset","updatedAt":42})" }) {
        for (const auto* key : { U"jacket", U"audio", U"chart" }) {
            auto json = Catalog();
            if (s3d::StringView{ key } == U"chart") {
                json[0][U"sheets"][0][key] = s3d::JSON::Parse(source);
            } else {
                json[0][U"music"][key] = s3d::JSON::Parse(source);
            }
            CHECK_THROWS_AS(api::ParseCatalog(json), s3d::Error);
        }
    }
}

TEST_CASE("Catalog rejects unknown difficulties", "[API][Catalog]") {
    auto json = Catalog();
    json[0][U"sheets"][0][U"difficulty"] = U"unknown";
    CHECK_THROWS_AS(api::ParseCatalog(json), s3d::Error);
}
