#include <ThirdParty/Catch2/catch.hpp>

#include "formats/sus/Parser.hpp"

namespace sus = xlair::sheets::formats::sus;

TEST_CASE("ParseText builds SUS timing data", "[SheetsAnalyzer][SUS][Parser]") {
    const auto result = sus::ParseText(UR"(#REQUEST "ticks_per_beat 960"
#REQUEST "enable_priority TRUE"
#BPM01: 120.0
#BPM0a: 180.5
#00002: 4
#00008: 01000A00
#MEASUREBS 1000
#00202: 3.5
#00208: 000a
)",
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
    const auto result = sus::ParseText(UR"(#00008: 01
#BPM01: 120
)",
                                       U"chart.sus");

    REQUIRE(result);
    REQUIRE(result->bpm_changes.size() == 1);
    CHECK(result->bpm_changes.front().definition == 1);
}

TEST_CASE("ParseText ignores metadata and note data not handled by the timing parser",
          "[SheetsAnalyzer][SUS][Parser]") {
    const auto result = sus::ParseText(UR"(#TITLE "Song"
#ARTIST "Artist"
#00010: 14141414
)",
                                       U"chart.sus");

    REQUIRE(result);
    CHECK(result->bpm_definitions.empty());
    CHECK(result->bpm_changes.isEmpty());
}

TEST_CASE("ParseText validates known REQUEST values and ignores unknown requests", "[SheetsAnalyzer][SUS][Parser]") {
    SECTION("disable priority") {
        const auto result = sus::ParseText(UR"(#REQUEST "enable_priority true"
#REQUEST "enable_priority false"
)",
                                           U"chart.sus");

        REQUIRE(result);
        CHECK_FALSE(result->priority_enabled);
    }

    SECTION("invalid priority value") {
        const auto result = sus::ParseText(UR"(#REQUEST "enable_priority maybe"
)",
                                           U"broken.sus");

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().message == U"#REQUEST enable_priority requires true or false.");
    }

    SECTION("unknown request") {
        const auto result = sus::ParseText(UR"(#REQUEST "future_option value"
)",
                                           U"chart.sus");

        REQUIRE(result);
        CHECK(result->ticks_per_beat == 480);
        CHECK_FALSE(result->priority_enabled);
    }
}

TEST_CASE("ParseText aggregates syntax and timing diagnostics", "[SheetsAnalyzer][SUS][Parser]") {
    const auto result = sus::ParseText(UR"(#
#REQUEST "ticks_per_beat 0"
#BPM00: 120
#BPM01: -1
#00002: 0
#00008: 01F
#00108: 02
)",
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
    const auto result = sus::ParseText(UR"(#MEASUREBS 4294967295
#00102: 4
)",
                                       U"broken.sus");

    REQUIRE_FALSE(result);
    REQUIRE(result.diagnostics.size() == 1);
    CHECK(*result.diagnostics.front().line == 2);
    CHECK(result.diagnostics.front().message == U"Measure number exceeds the uint32 range after applying #MEASUREBS.");
}

TEST_CASE("ParseText reports the source column of invalid BPM data", "[SheetsAnalyzer][SUS][Parser]") {
    const auto result = sus::ParseText(UR"(#00008: 0!
)",
                                       U"broken.sus");

    REQUIRE_FALSE(result);
    REQUIRE(result.diagnostics.size() == 1);
    CHECK(*result.diagnostics.front().line == 1);
    CHECK(*result.diagnostics.front().column == 10);
}

TEST_CASE("ParseText builds hispeed definitions from measure and tick positions", "[SheetsAnalyzer][SUS][Parser]") {
    const auto result = sus::ParseText(UR"(#TIL00: "0'0:1.0, 2'960:-0.5, 3'0:0"
#HISPEED 00
#NOSPEED
)",
                                       U"chart.sus");

    REQUIRE(result);
    REQUIRE(result->hispeed_definitions.size() == 1);
    const auto& definition = result->hispeed_definitions.at(0);
    REQUIRE(definition.changes.size() == 3);
    CHECK(definition.changes[0].position.measure == 0);
    CHECK(definition.changes[0].position.tick == 0);
    CHECK(definition.changes[0].multiplier == 1.0);
    CHECK(definition.changes[1].position.measure == 2);
    CHECK(definition.changes[1].position.tick == 960);
    CHECK(definition.changes[1].multiplier == -0.5);
    CHECK(definition.changes[2].position.measure == 3);
    CHECK(definition.changes[2].position.tick == 0);
    CHECK(definition.changes[2].multiplier == 0.0);
}

TEST_CASE("ParseText resolves hispeed definitions declared after selection", "[SheetsAnalyzer][SUS][Parser]") {
    const auto result = sus::ParseText(UR"(#HISPEED 0a
#TIL0A: "0'0:1"
)",
                                       U"chart.sus");

    REQUIRE(result);
    CHECK(result->hispeed_definitions.contains(10));
}

TEST_CASE("ParseText validates hispeed definitions and selections", "[SheetsAnalyzer][SUS][Parser]") {
    SECTION("malformed change") {
        const auto result = sus::ParseText(UR"(#TIL01: "0:1"
)",
                                           U"broken.sus");

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().message == U"Hispeed changes must use the form measure'tick:speed.");
    }

    SECTION("invalid numeric value") {
        const auto result = sus::ParseText(UR"(#TIL01: "-1'0:1"
)",
                                           U"broken.sus");

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().message ==
              U"Hispeed changes require a non-negative measure and tick and a finite speed.");
    }

    SECTION("undefined selection") {
        const auto result = sus::ParseText(UR"(#HISPEED ZZ
)",
                                           U"broken.sus");

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().message == U"#HISPEED references an undefined hispeed definition.");
    }

    SECTION("NOSPEED argument") {
        const auto result = sus::ParseText(UR"(#NOSPEED 01
)",
                                           U"broken.sus");

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().message == U"#NOSPEED does not accept an argument.");
    }
}
