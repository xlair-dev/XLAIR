#include <ThirdParty/Catch2/catch.hpp>

#include "formats/sus/Compiler.hpp"

namespace sus = xlair::sheets::formats::sus;

TEST_CASE("SideLong start lanes map to XLAIR side buttons", "[SheetsAnalyzer][SUS][Compiler]") {
    const auto check_mapping = [](const s3d::uint8 lane, const xlair::sheets::SideButton expected) {
        const auto button = sus::detail::SideButtonFromSideLongLane(lane);
        REQUIRE(button.has_value());
        CHECK(*button == expected);
    };

    check_mapping(0, xlair::sheets::SideButton::LeftUpper);
    check_mapping(1, xlair::sheets::SideButton::LeftUpper);
    check_mapping(2, xlair::sheets::SideButton::LeftLower);
    check_mapping(3, xlair::sheets::SideButton::LeftLower);
    check_mapping(12, xlair::sheets::SideButton::RightLower);
    check_mapping(13, xlair::sheets::SideButton::RightLower);
    check_mapping(14, xlair::sheets::SideButton::RightUpper);
    check_mapping(15, xlair::sheets::SideButton::RightUpper);

    for (s3d::uint8 lane = 4; lane <= 11; ++lane) {
        CHECK_FALSE(sus::detail::SideButtonFromSideLongLane(lane).has_value());
    }
    CHECK_FALSE(sus::detail::SideButtonFromSideLongLane(16).has_value());
}

TEST_CASE("Compile builds timing, timelines, and simple notes", "[SheetsAnalyzer][SUS][Compiler]") {
    sus::Document document;
    document.bpm_definitions[1] = 60.0;
    document.bpm_changes.push_back({
        .position = { .measure = 0, .numerator = 1, .denominator = 2 },
        .definition = 1,
    });
    document.hispeed_definitions[2] = {};
    document.hispeed_definitions[10].changes = {
        {
            .position = { .measure = 1, .tick = 0 },
            .multiplier = 2.0,
        },
        {
            .position = { .measure = 0, .tick = 480 },
            .multiplier = 0.5,
        },
    };
    document.slider_notes = {
        {
            .kind = sus::SliderNoteKind::Tap1,
            .position = { .measure = 1, .numerator = 0, .denominator = 1 },
            .lane = { .start = 4, .width = 4 },
            .timeline = 10,
        },
        {
            .kind = sus::SliderNoteKind::Tap2,
            .position = { .measure = 0, .numerator = 1, .denominator = 4 },
            .lane = { .start = 2, .width = 2 },
            .timeline = s3d::none,
        },
    };
    document.directional_notes.push_back({
        .kind = sus::DirectionalKind::LeftUp,
        .position = { .measure = 0, .numerator = 1, .denominator = 2 },
        .lane = { .start = 0, .width = 2 },
        .timeline = 10,
    });

    const auto result = sus::Compile(document, {
                                                   .sample_rate = 480,
                                                   .offset_seconds = 0.25,
                                               });

    REQUIRE(result);
    CHECK(result->sample_rate == 480);

    REQUIRE(result->tempo_changes.size() == 2);
    CHECK(result->tempo_changes[0].sample == 120);
    CHECK(result->tempo_changes[0].bpm == 120.0);
    CHECK(result->tempo_changes[1].sample == 600);
    CHECK(result->tempo_changes[1].bpm == 60.0);

    REQUIRE(result->timelines.size() == 3);
    REQUIRE(result->timelines[0].speed_changes.size() == 1);
    CHECK(result->timelines[0].speed_changes[0].sample == 120);
    CHECK(result->timelines[0].speed_changes[0].multiplier == 1.0);
    REQUIRE(result->timelines[1].speed_changes.size() == 1);
    REQUIRE(result->timelines[2].speed_changes.size() == 3);
    CHECK(result->timelines[2].speed_changes[0].sample == 120);
    CHECK(result->timelines[2].speed_changes[0].multiplier == 1.0);
    CHECK(result->timelines[2].speed_changes[1].sample == 360);
    CHECK(result->timelines[2].speed_changes[1].multiplier == 0.5);
    CHECK(result->timelines[2].speed_changes[2].sample == 1'560);
    CHECK(result->timelines[2].speed_changes[2].multiplier == 2.0);

    REQUIRE(result->slider_notes.size() == 2);
    CHECK(result->slider_notes[0].kind == xlair::sheets::SliderNoteKind::XTap);
    CHECK(result->slider_notes[0].timeline == 0);
    CHECK(result->slider_notes[0].sample == 360);
    CHECK(result->slider_notes[0].lane.start == 2);
    CHECK(result->slider_notes[0].lane.width == 2);
    CHECK(result->slider_notes[1].kind == xlair::sheets::SliderNoteKind::Tap);
    CHECK(result->slider_notes[1].timeline == 2);
    CHECK(result->slider_notes[1].sample == 1'560);

    REQUIRE(result->side_notes.size() == 1);
    CHECK(result->side_notes[0].timeline == 2);
    CHECK(result->side_notes[0].sample == 600);
    CHECK(result->side_notes[0].button == xlair::sheets::SideButton::LeftUpper);
    CHECK(result->total_combo == 3);
}

TEST_CASE("Compile rejects source data that cannot yet be represented safely", "[SheetsAnalyzer][SUS][Compiler]") {
    SECTION("unsupported SUS short-note kind") {
        sus::Document document;
        document.slider_notes.push_back({ .kind = sus::SliderNoteKind::Tap4 });

        const auto result = sus::Compile(document, {});

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().message ==
              U"SUS short-note kinds 4 through 6 are not supported by XLAIR.");
    }

    SECTION("undefined timeline") {
        sus::Document document;
        document.slider_notes.push_back({ .timeline = 99 });

        const auto result = sus::Compile(document, {});

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().message == U"A slider note references an undefined hispeed definition.");
    }

    SECTION("unsupported directional") {
        sus::Document document;
        document.directional_notes.push_back({ .kind = sus::DirectionalKind::Up });

        const auto result = sus::Compile(document, {});

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().message == U"Up and down directional notes are not supported by XLAIR.");
    }

    SECTION("hold notes") {
        sus::Document document;
        document.slider_hold_points.push_back({});

        const auto result = sus::Compile(document, {});

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().message == U"Slider Hold and SideLong compilation has not been migrated yet.");
    }
}
