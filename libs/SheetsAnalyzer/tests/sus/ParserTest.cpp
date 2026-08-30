#include <ThirdParty/Catch2/catch.hpp>

#include "formats/sus/Parser.hpp"

namespace sus = xlair::sheets::formats::sus;

TEST_CASE("ParseText builds SUS timing data", "[SheetsAnalyzer][SUS][Parser]") {
    const auto result = sus::ParseText(
        UR"(#REQUEST "ticks_per_beat 960"
#REQUEST "enable_priority TRUE"
#BPM01: 120.0
#BPM0a: 180.5
#00002: 4
#00008: 01000A00
#MEASUREBS 1000
#00202: 3.5
#00208: 000a
)",
        U"chart.sus"
    );

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
    const auto result = sus::ParseText(
        UR"(#00008: 01
#BPM01: 120
)",
        U"chart.sus"
    );

    REQUIRE(result);
    REQUIRE(result->bpm_changes.size() == 1);
    CHECK(result->bpm_changes.front().definition == 1);
}

TEST_CASE("ParseText ignores metadata outside the parser's current scope", "[SheetsAnalyzer][SUS][Parser]") {
    const auto result = sus::ParseText(
        UR"(#TITLE "Song"
#ARTIST "Artist"
)",
        U"chart.sus"
    );

    REQUIRE(result);
    CHECK(result->bpm_definitions.empty());
    CHECK(result->bpm_changes.isEmpty());
}

TEST_CASE("ParseText validates known REQUEST values and ignores unknown requests", "[SheetsAnalyzer][SUS][Parser]") {
    SECTION("disable priority") {
        const auto result = sus::ParseText(
            UR"(#REQUEST "enable_priority true"
#REQUEST "enable_priority false"
)",
            U"chart.sus"
        );

        REQUIRE(result);
        CHECK_FALSE(result->priority_enabled);
    }

    SECTION("invalid priority value") {
        const auto result = sus::ParseText(
            UR"(#REQUEST "enable_priority maybe"
)",
            U"broken.sus"
        );

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().message == U"#REQUEST enable_priority requires true or false.");
    }

    SECTION("unknown request") {
        const auto result = sus::ParseText(
            UR"(#REQUEST "future_option value"
)",
            U"chart.sus"
        );

        REQUIRE(result);
        CHECK(result->ticks_per_beat == 480);
        CHECK_FALSE(result->priority_enabled);
    }
}

TEST_CASE(
    "ParseText builds short and directional notes with the active hispeed timeline", "[SheetsAnalyzer][SUS][Parser]"
) {
    const auto result = sus::ParseText(
        UR"(#TIL01: "0'0:1"
#HISPEED 01
#MEASUREBS 1000
#0101A: 1Z0024003400
#0105a: 142434445464
#NOSPEED
#01110: 14
)",
        U"chart.sus"
    );

    REQUIRE(result);
    REQUIRE(result->slider_notes.size() == 4);

    const auto& tap1 = result->slider_notes[0];
    CHECK(tap1.kind == sus::SliderNoteKind::Tap1);
    CHECK(tap1.position.measure == 1010);
    CHECK(tap1.position.numerator == 0);
    CHECK(tap1.position.denominator == 6);
    CHECK(tap1.lane.start == 10);
    CHECK(tap1.lane.width == 35);
    REQUIRE(tap1.timeline);
    CHECK(*tap1.timeline == 1);

    const auto& tap2 = result->slider_notes[1];
    CHECK(tap2.kind == sus::SliderNoteKind::Tap2);
    CHECK(tap2.position.numerator == 2);
    CHECK(tap2.position.denominator == 6);

    const auto& tap3 = result->slider_notes[2];
    CHECK(tap3.kind == sus::SliderNoteKind::Tap3);
    CHECK(tap3.position.numerator == 4);
    CHECK(tap3.position.denominator == 6);

    const auto& note_without_hispeed = result->slider_notes[3];
    CHECK(note_without_hispeed.position.measure == 1011);
    CHECK_FALSE(note_without_hispeed.timeline);

    REQUIRE(result->directional_notes.size() == 6);
    const s3d::Array expected_kinds{
        sus::DirectionalKind::Up,      sus::DirectionalKind::Down,     sus::DirectionalKind::LeftUp,
        sus::DirectionalKind::RightUp, sus::DirectionalKind::LeftDown, sus::DirectionalKind::RightDown,
    };
    for (std::size_t index = 0; index < expected_kinds.size(); ++index) {
        const auto& note = result->directional_notes[index];
        CHECK(note.kind == expected_kinds[index]);
        CHECK(note.position.measure == 1010);
        CHECK(note.position.numerator == index);
        CHECK(note.position.denominator == 6);
        CHECK(note.lane.start == 10);
        CHECK(note.lane.width == 4);
        REQUIRE(note.timeline);
        CHECK(*note.timeline == 1);
    }
}

TEST_CASE("ParseText preserves all six SUS short-note kinds", "[SheetsAnalyzer][SUS][Parser]") {
    const auto result = sus::ParseText(
        UR"(#00010: 142434445464
)",
        U"chart.sus"
    );

    REQUIRE(result);
    REQUIRE(result->slider_notes.size() == 6);

    const s3d::Array expected_kinds{
        sus::SliderNoteKind::Tap1, sus::SliderNoteKind::Tap2, sus::SliderNoteKind::Tap3,
        sus::SliderNoteKind::Tap4, sus::SliderNoteKind::Tap5, sus::SliderNoteKind::Tap6,
    };
    for (std::size_t index = 0; index < expected_kinds.size(); ++index) {
        CHECK(result->slider_notes[index].kind == expected_kinds[index]);
    }
}

TEST_CASE("ParseText validates short and directional note data", "[SheetsAnalyzer][SUS][Parser]") {
    SECTION("odd-length note data") {
        const auto result = sus::ParseText(
            UR"(#00010: 1
)",
            U"broken.sus"
        );

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().message == U"Note data must contain two characters per subdivision.");
    }

    SECTION("invalid base36 width") {
        const auto result = sus::ParseText(
            UR"(#00010: 1!
)",
            U"broken.sus"
        );

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(*result.diagnostics.front().column == 10);
    }

    SECTION("zero width") {
        const auto result = sus::ParseText(
            UR"(#00010: 10
)",
            U"broken.sus"
        );

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().message == U"Placed notes require a width between 1 and Z.");
    }

    SECTION("unsupported short note kind") {
        const auto result = sus::ParseText(
            UR"(#00010: 74
)",
            U"broken.sus"
        );

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().message == U"Short note kinds must be between 1 and 6.");
    }

    SECTION("unsupported directional note kind") {
        const auto result = sus::ParseText(
            UR"(#00050: 74
)",
            U"broken.sus"
        );

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().message == U"Directional note kinds must be between 1 and 6.");
    }

    SECTION("invalid short note header") {
        const auto result = sus::ParseText(
            UR"(#000100: 14
)",
            U"broken.sus"
        );

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().message == U"Short note headers must use the form #mmm1x.");
    }
}

TEST_CASE(
    "ParseText builds slider hold points with their channel and active timeline", "[SheetsAnalyzer][SUS][Parser]"
) {
    const auto result = sus::ParseText(
        UR"(#TIL0A: "0'0:1"
#HISPEED 0A
#02030a: 140034004400550024
#NOSPEED
#0213BZ: 1525
)",
        U"chart.sus"
    );

    REQUIRE(result);
    REQUIRE(result->slider_hold_points.size() == 7);

    const s3d::Array expected_kinds{
        sus::SliderHoldPointKind::Start,     sus::SliderHoldPointKind::Visible, sus::SliderHoldPointKind::Control,
        sus::SliderHoldPointKind::Invisible, sus::SliderHoldPointKind::End,
    };
    const s3d::Array<s3d::uint32> expected_numerators{ 0, 2, 4, 6, 8 };
    for (std::size_t index = 0; index < expected_kinds.size(); ++index) {
        const auto& point = result->slider_hold_points[index];
        CHECK(point.kind == expected_kinds[index]);
        CHECK(point.position.measure == 20);
        CHECK(point.position.numerator == expected_numerators[index]);
        CHECK(point.position.denominator == 9);
        CHECK(point.lane.start == 0);
        CHECK(point.lane.width == (index == 3 ? 5 : 4));
        CHECK(point.channel == 10);
        REQUIRE(point.timeline);
        CHECK(*point.timeline == 10);
    }

    const auto& second_start = result->slider_hold_points[5];
    CHECK(second_start.kind == sus::SliderHoldPointKind::Start);
    CHECK(second_start.position.measure == 21);
    CHECK(second_start.position.numerator == 0);
    CHECK(second_start.position.denominator == 2);
    CHECK(second_start.lane.start == 11);
    CHECK(second_start.lane.width == 5);
    CHECK(second_start.channel == 35);
    CHECK_FALSE(second_start.timeline);

    const auto& second_end = result->slider_hold_points[6];
    CHECK(second_end.kind == sus::SliderHoldPointKind::End);
    CHECK(second_end.position.numerator == 1);
    CHECK(second_end.position.denominator == 2);
}

TEST_CASE("ParseText validates slider hold point data", "[SheetsAnalyzer][SUS][Parser]") {
    SECTION("unsupported point kind") {
        const auto result = sus::ParseText(
            UR"(#000300: 64
)",
            U"broken.sus"
        );

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().message == U"Slider hold point kinds must be between 1 and 5.");
    }

    SECTION("invalid header") {
        const auto result = sus::ParseText(
            UR"(#00030: 14
)",
            U"broken.sus"
        );

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().message == U"Slider hold headers must use the form #mmm3xy.");
    }
}

TEST_CASE("ParseText builds SideLong points with their channel and active timeline", "[SheetsAnalyzer][SUS][Parser]") {
    const auto result = sus::ParseText(
        UR"(#TIL01: "0'0:1"
#HISPEED 01
#00020A: 1400
#00125a: 0034
#0022Fa: 0024
#NOSPEED
#0032CZ: 1424
)",
        U"chart.sus"
    );

    REQUIRE(result);
    REQUIRE(result->side_long_points.size() == 5);

    const auto& start = result->side_long_points[0];
    CHECK(start.kind == sus::SideLongPointKind::Start);
    CHECK(start.position.measure == 0);
    CHECK(start.position.numerator == 0);
    CHECK(start.position.denominator == 2);
    CHECK(start.lane.start == 0);
    CHECK(start.lane.width == 4);
    CHECK(start.channel == 10);
    REQUIRE(start.timeline);
    CHECK(*start.timeline == 1);

    const auto& relay = result->side_long_points[1];
    CHECK(relay.kind == sus::SideLongPointKind::Relay);
    CHECK(relay.position.measure == 1);
    CHECK(relay.position.numerator == 1);
    CHECK(relay.position.denominator == 2);
    CHECK(relay.lane.start == 5);
    CHECK(relay.channel == 10);

    const auto& end = result->side_long_points[2];
    CHECK(end.kind == sus::SideLongPointKind::End);
    CHECK(end.position.measure == 2);
    CHECK(end.position.numerator == 1);
    CHECK(end.position.denominator == 2);
    CHECK(end.lane.start == 15);
    CHECK(end.channel == 10);

    const auto& second_start = result->side_long_points[3];
    CHECK(second_start.kind == sus::SideLongPointKind::Start);
    CHECK(second_start.lane.start == 12);
    CHECK(second_start.channel == 35);
    CHECK_FALSE(second_start.timeline);

    const auto& second_end = result->side_long_points[4];
    CHECK(second_end.kind == sus::SideLongPointKind::End);
    CHECK(second_end.position.numerator == 1);
    CHECK(second_end.position.denominator == 2);
    CHECK(second_end.channel == 35);
}

TEST_CASE("ParseText validates SideLong point data", "[SheetsAnalyzer][SUS][Parser]") {
    SECTION("unsupported point kind") {
        const auto result = sus::ParseText(
            UR"(#000200: 44
)",
            U"broken.sus"
        );

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().message == U"SideLong point kinds must be 1 (Start), 2 (End), or 3 (Relay).");
    }

    SECTION("invalid header") {
        const auto result = sus::ParseText(
            UR"(#00020: 14
)",
            U"broken.sus"
        );

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().message == U"SideLong headers must use the form #mmm2xy.");
    }
}

TEST_CASE("ParseText aggregates syntax and timing diagnostics", "[SheetsAnalyzer][SUS][Parser]") {
    const auto result = sus::ParseText(
        UR"(#
#REQUEST "ticks_per_beat 0"
#BPM00: 120
#BPM01: -1
#00002: 0
#00008: 01F
#00108: 02
)",
        U"broken.sus"
    );

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
    const auto result = sus::ParseText(
        UR"(#MEASUREBS 4294967295
#00102: 4
)",
        U"broken.sus"
    );

    REQUIRE_FALSE(result);
    REQUIRE(result.diagnostics.size() == 1);
    CHECK(*result.diagnostics.front().line == 2);
    CHECK(result.diagnostics.front().message == U"Measure number exceeds the uint32 range after applying #MEASUREBS.");
}

TEST_CASE("ParseText reports the source column of invalid BPM data", "[SheetsAnalyzer][SUS][Parser]") {
    const auto result = sus::ParseText(
        UR"(#00008: 0!
)",
        U"broken.sus"
    );

    REQUIRE_FALSE(result);
    REQUIRE(result.diagnostics.size() == 1);
    CHECK(*result.diagnostics.front().line == 1);
    CHECK(*result.diagnostics.front().column == 10);
}

TEST_CASE("ParseText builds hispeed definitions from measure and tick positions", "[SheetsAnalyzer][SUS][Parser]") {
    const auto result = sus::ParseText(
        UR"(#TIL00: "0'0:1.0, 2'960:-0.5, 3'0:0"
#HISPEED 00
#NOSPEED
)",
        U"chart.sus"
    );

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

TEST_CASE("ParseText accepts an empty hispeed definition", "[SheetsAnalyzer][SUS][Parser]") {
    const auto result = sus::ParseText(
        UR"(#TIL00: ""
#HISPEED 00
)",
        U"chart.sus"
    );

    REQUIRE(result);
    REQUIRE(result->hispeed_definitions.contains(0));
    CHECK(result->hispeed_definitions.at(0).changes.isEmpty());
}

TEST_CASE("ParseText resolves hispeed definitions declared after selection", "[SheetsAnalyzer][SUS][Parser]") {
    const auto result = sus::ParseText(
        UR"(#HISPEED 0a
#TIL0A: "0'0:1"
)",
        U"chart.sus"
    );

    REQUIRE(result);
    CHECK(result->hispeed_definitions.contains(10));
}

TEST_CASE("ParseText validates hispeed definitions and selections", "[SheetsAnalyzer][SUS][Parser]") {
    SECTION("malformed change") {
        const auto result = sus::ParseText(
            UR"(#TIL01: "0:1"
)",
            U"broken.sus"
        );

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().message == U"Hispeed changes must use the form measure'tick:speed.");
    }

    SECTION("invalid numeric value") {
        const auto result = sus::ParseText(
            UR"(#TIL01: "-1'0:1"
)",
            U"broken.sus"
        );

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(
            result.diagnostics.front().message ==
            U"Hispeed changes require a non-negative measure and tick and a finite speed."
        );
    }

    SECTION("undefined selection") {
        const auto result = sus::ParseText(
            UR"(#HISPEED ZZ
)",
            U"broken.sus"
        );

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().message == U"#HISPEED references an undefined hispeed definition.");
    }

    SECTION("NOSPEED argument") {
        const auto result = sus::ParseText(
            UR"(#NOSPEED 01
)",
            U"broken.sus"
        );

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().message == U"#NOSPEED does not accept an argument.");
    }
}
