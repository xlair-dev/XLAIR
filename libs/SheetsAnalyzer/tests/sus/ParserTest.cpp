#include <ThirdParty/Catch2/catch.hpp>

#include "formats/sus/Parser.hpp"

namespace sus = xlair::sheets::formats::sus;

TEST_CASE("ParseText builds SUS timing data", "[SheetsAnalyzer][SUS][Parser]") {
    const auto result = sus::ParseText(U"#REQUEST \"ticks_per_beat 960\"\n"
                                       U"#REQUEST \"enable_priority TRUE\"\n"
                                       U"#BPM01: 120.0\n"
                                       U"#BPM0a: 180.5\n"
                                       U"#00002: 4\n"
                                       U"#00008: 01000A00\n"
                                       U"#MEASUREBS 1000\n"
                                       U"#00202: 3.5\n"
                                       U"#00208: 000a\n",
                                       U"chart.sus");

    REQUIRE(result);
    CHECK(result->ticks_per_beat == 960);
    CHECK(result->priority_enabled);
    REQUIRE(result->bpm_definitions.size() == 2);
    CHECK(result->bpm_definitions.at(1) == 120.0);
    CHECK(result->bpm_definitions.at(10) == 180.5);
    REQUIRE(result->beats_per_measure.size() == 2);
    CHECK(result->beats_per_measure.at(0) == 4.0);
    CHECK(result->beats_per_measure.at(1002) == 3.5);

    REQUIRE(result->bpm_changes.size() == 3);
    CHECK(result->bpm_changes[0].position.measure == 0);
    CHECK(result->bpm_changes[0].position.numerator == 0);
    CHECK(result->bpm_changes[0].position.denominator == 4);
    CHECK(result->bpm_changes[0].definition == 1);
    CHECK(result->bpm_changes[1].position.measure == 0);
    CHECK(result->bpm_changes[1].position.numerator == 2);
    CHECK(result->bpm_changes[1].position.denominator == 4);
    CHECK(result->bpm_changes[1].definition == 10);
    CHECK(result->bpm_changes[2].position.measure == 1002);
    CHECK(result->bpm_changes[2].position.numerator == 1);
    CHECK(result->bpm_changes[2].position.denominator == 2);
    CHECK(result->bpm_changes[2].definition == 10);
}

TEST_CASE("ParseText resolves BPM definitions declared after their use", "[SheetsAnalyzer][SUS][Parser]") {
    const auto result = sus::ParseText(U"#00008: 01\n"
                                       U"#BPM01: 120\n",
                                       U"chart.sus");

    REQUIRE(result);
    REQUIRE(result->bpm_changes.size() == 1);
    CHECK(result->bpm_changes.front().definition == 1);
}

TEST_CASE("ParseText ignores metadata and note data not handled by the timing parser",
          "[SheetsAnalyzer][SUS][Parser]") {
    const auto result = sus::ParseText(U"#TITLE \"Song\"\n"
                                       U"#ARTIST \"Artist\"\n"
                                       U"#00010: 14141414\n",
                                       U"chart.sus");

    REQUIRE(result);
    CHECK(result->bpm_definitions.empty());
    CHECK(result->bpm_changes.isEmpty());
}

TEST_CASE("ParseText validates known REQUEST values and ignores unknown requests", "[SheetsAnalyzer][SUS][Parser]") {
    SECTION("disable priority") {
        const auto result = sus::ParseText(U"#REQUEST \"enable_priority true\"\n"
                                           U"#REQUEST \"enable_priority false\"\n",
                                           U"chart.sus");

        REQUIRE(result);
        CHECK_FALSE(result->priority_enabled);
    }

    SECTION("invalid priority value") {
        const auto result = sus::ParseText(U"#REQUEST \"enable_priority maybe\"\n", U"broken.sus");

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().message == U"#REQUEST enable_priority requires true or false.");
    }

    SECTION("unknown request") {
        const auto result = sus::ParseText(U"#REQUEST \"future_option value\"\n", U"chart.sus");

        REQUIRE(result);
        CHECK(result->ticks_per_beat == 480);
        CHECK_FALSE(result->priority_enabled);
    }
}

TEST_CASE("ParseText aggregates syntax and timing diagnostics", "[SheetsAnalyzer][SUS][Parser]") {
    const auto result = sus::ParseText(U"#\n"
                                       U"#REQUEST \"ticks_per_beat 0\"\n"
                                       U"#BPM00: 120\n"
                                       U"#BPM01: -1\n"
                                       U"#00002: 0\n"
                                       U"#00008: 01F\n"
                                       U"#00108: 02\n",
                                       U"broken.sus");

    REQUIRE_FALSE(result);
    CHECK(result.diagnostics.size() == 7);
    CHECK(*result.diagnostics[0].line == 1);
    CHECK(*result.diagnostics[1].line == 2);
    CHECK(*result.diagnostics[2].line == 3);
    CHECK(*result.diagnostics[3].line == 4);
    CHECK(*result.diagnostics[4].line == 5);
    CHECK(*result.diagnostics[5].line == 6);
    CHECK(*result.diagnostics[6].line == 7);
    CHECK(result.diagnostics[6].message == U"BPM change references an undefined BPM ID.");
}

TEST_CASE("ParseText applies the latest measure base and detects overflow", "[SheetsAnalyzer][SUS][Parser]") {
    const auto result = sus::ParseText(U"#MEASUREBS 4294967295\n"
                                       U"#00102: 4\n",
                                       U"broken.sus");

    REQUIRE_FALSE(result);
    REQUIRE(result.diagnostics.size() == 1);
    CHECK(*result.diagnostics.front().line == 2);
    CHECK(result.diagnostics.front().message == U"Measure number exceeds the uint32 range after applying #MEASUREBS.");
}

TEST_CASE("ParseText reports the source column of invalid BPM data", "[SheetsAnalyzer][SUS][Parser]") {
    const auto result = sus::ParseText(U"#00008: 0!\n", U"broken.sus");

    REQUIRE_FALSE(result);
    REQUIRE(result.diagnostics.size() == 1);
    CHECK(*result.diagnostics.front().line == 1);
    CHECK(*result.diagnostics.front().column == 10);
}
