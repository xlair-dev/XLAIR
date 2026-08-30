#include <ThirdParty/Catch2/catch.hpp>

#include "formats/sus/Compiler.hpp"

namespace sus = xlair::sheets::formats::sus;

TEST_CASE("Compile assembles interleaved Slider Hold channels in musical order",
          "[SheetsAnalyzer][SUS][Compiler]") {
    sus::Document document;
    document.hispeed_definitions[1] = {};
    document.slider_hold_points = {
        {
            .kind = sus::SliderHoldPointKind::Visible,
            .position = { .measure = 0, .numerator = 1, .denominator = 2 },
            .lane = { .start = 4, .width = 2 },
            .channel = 2,
            .timeline = 1,
        },
        {
            .kind = sus::SliderHoldPointKind::End,
            .position = { .measure = 1, .numerator = 0, .denominator = 1 },
            .lane = { .start = 8, .width = 2 },
            .channel = 1,
        },
        {
            .kind = sus::SliderHoldPointKind::End,
            .position = { .measure = 1, .numerator = 1, .denominator = 2 },
            .lane = { .start = 6, .width = 3 },
            .channel = 2,
            .timeline = 1,
        },
        {
            .kind = sus::SliderHoldPointKind::Start,
            .position = { .measure = 0, .numerator = 0, .denominator = 1 },
            .lane = { .start = 0, .width = 4 },
            .channel = 1,
        },
        {
            .kind = sus::SliderHoldPointKind::Start,
            .position = { .measure = 0, .numerator = 1, .denominator = 4 },
            .lane = { .start = 2, .width = 2 },
            .channel = 2,
            .timeline = 1,
        },
        {
            .kind = sus::SliderHoldPointKind::Control,
            .position = { .measure = 0, .numerator = 1, .denominator = 4 },
            .lane = { .start = 1, .width = 3 },
            .channel = 1,
        },
        {
            .kind = sus::SliderHoldPointKind::Invisible,
            .position = { .measure = 0, .numerator = 3, .denominator = 4 },
            .lane = { .start = 4, .width = 2 },
            .channel = 1,
        },
    };

    const auto result = sus::Compile(document, { .sample_rate = 480 });

    REQUIRE(result);
    REQUIRE(result->slider_holds.size() == 2);

    const auto& first = result->slider_holds[0];
    REQUIRE(first.points.size() == 4);
    CHECK(first.points[0].kind == xlair::sheets::SliderHoldPointKind::Start);
    CHECK(first.points[0].timeline == 0);
    CHECK(first.points[0].sample == 0);
    CHECK(first.points[0].lane.start == 0);
    CHECK(first.points[0].lane.width == 4);
    CHECK(first.points[1].kind == xlair::sheets::SliderHoldPointKind::Control);
    CHECK(first.points[1].sample == 240);
    CHECK(first.points[1].lane.start == 1);
    CHECK(first.points[1].lane.width == 3);
    CHECK(first.points[2].kind == xlair::sheets::SliderHoldPointKind::Invisible);
    CHECK(first.points[2].sample == 720);
    CHECK(first.points[3].kind == xlair::sheets::SliderHoldPointKind::End);
    CHECK(first.points[3].sample == 960);
    CHECK(first.judge_points.isEmpty());

    const auto& second = result->slider_holds[1];
    REQUIRE(second.points.size() == 3);
    CHECK(second.points[0].kind == xlair::sheets::SliderHoldPointKind::Start);
    CHECK(second.points[0].timeline == 1);
    CHECK(second.points[0].sample == 240);
    CHECK(second.points[1].kind == xlair::sheets::SliderHoldPointKind::Visible);
    CHECK(second.points[1].timeline == 1);
    CHECK(second.points[1].sample == 480);
    CHECK(second.points[1].lane.start == 4);
    CHECK(second.points[1].lane.width == 2);
    CHECK(second.points[2].kind == xlair::sheets::SliderHoldPointKind::End);
    CHECK(second.points[2].sample == 1'440);
    CHECK(second.judge_points.isEmpty());
}

TEST_CASE("Compile validates Slider Hold channel state", "[SheetsAnalyzer][SUS][Compiler]") {
    SECTION("duplicate Start") {
        sus::Document document;
        document.slider_hold_points = {
            { .kind = sus::SliderHoldPointKind::Start, .channel = 1 },
            {
                .kind = sus::SliderHoldPointKind::Start,
                .position = { .measure = 0, .numerator = 1, .denominator = 2 },
                .channel = 1,
            },
        };

        const auto result = sus::Compile(document, {});

        REQUIRE_FALSE(result);
        CHECK(result.diagnostics.front().message ==
              U"A Slider Hold channel starts before its previous hold ends.");
    }

    SECTION("intermediate point without Start") {
        sus::Document document;
        document.slider_hold_points.push_back({ .kind = sus::SliderHoldPointKind::Visible, .channel = 1 });

        const auto result = sus::Compile(document, {});

        REQUIRE_FALSE(result);
        CHECK(result.diagnostics.front().message ==
              U"A Slider Hold point has no active Start point on its channel.");
    }

    SECTION("End without Start") {
        sus::Document document;
        document.slider_hold_points.push_back({ .kind = sus::SliderHoldPointKind::End, .channel = 1 });

        const auto result = sus::Compile(document, {});

        REQUIRE_FALSE(result);
        CHECK(result.diagnostics.front().message ==
              U"A Slider Hold End point has no active Start point on its channel.");
    }

    SECTION("missing End") {
        sus::Document document;
        document.slider_hold_points.push_back({ .kind = sus::SliderHoldPointKind::Start, .channel = 1 });

        const auto result = sus::Compile(document, {});

        REQUIRE_FALSE(result);
        CHECK(result.diagnostics.front().message == U"A Slider Hold channel is missing an End point.");
    }

    SECTION("undefined timeline") {
        sus::Document document;
        document.slider_hold_points.push_back({
            .kind = sus::SliderHoldPointKind::Start,
            .channel = 1,
            .timeline = 99,
        });

        const auto result = sus::Compile(document, {});

        REQUIRE_FALSE(result);
        CHECK(result.diagnostics.front().message ==
              U"A Slider Hold point references an undefined hispeed definition.");
    }
}
