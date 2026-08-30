#include <ThirdParty/Catch2/catch.hpp>

#include "formats/sus/Compiler.hpp"

#include <algorithm>

namespace sus = xlair::sheets::formats::sus;

TEST_CASE("Compile assembles interleaved Slider Hold channels in musical order", "[SheetsAnalyzer][SUS][Compiler]") {
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
    REQUIRE(first.points.size() > 3);
    CHECK(first.points.front().kind == xlair::sheets::SliderHoldPointKind::Start);
    CHECK(first.points.front().timeline == 0);
    CHECK(first.points.front().sample == 0);
    CHECK(first.points.front().lane.start == 0);
    CHECK(first.points.front().lane.width == 4);
    CHECK(first.points.back().kind == xlair::sheets::SliderHoldPointKind::End);
    CHECK(first.points.back().sample == 960);
    for (std::size_t index = 1; index < first.points.size(); ++index) {
        CHECK(first.points[index - 1].sample < first.points[index].sample);
    }
    REQUIRE(first.judge_points.size() == 9);
    for (std::size_t index = 0; index < first.judge_points.size(); ++index) {
        CHECK(first.judge_points[index].sample == static_cast<s3d::int64>(index * 120));
    }

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
    REQUIRE(second.judge_points.size() == 11);
    for (std::size_t index = 0; index < second.judge_points.size(); ++index) {
        CHECK(second.judge_points[index].sample == 240 + static_cast<s3d::int64>(index * 120));
    }
}

TEST_CASE(
    "Compile expands Slider Hold Control points into generated Invisible points", "[SheetsAnalyzer][SUS][Compiler]"
) {
    sus::Document document;
    document.slider_hold_points = {
        {
            .kind = sus::SliderHoldPointKind::Start,
            .lane = { .start = 0, .width = 2 },
            .channel = 1,
        },
        {
            .kind = sus::SliderHoldPointKind::Control,
            .position = { .measure = 0, .numerator = 1, .denominator = 2 },
            .lane = { .start = 13, .width = 2 },
            .channel = 1,
        },
        {
            .kind = sus::SliderHoldPointKind::End,
            .position = { .measure = 1 },
            .lane = { .start = 0, .width = 2 },
            .channel = 1,
        },
    };

    const auto result = sus::Compile(document, { .sample_rate = 100 });

    REQUIRE(result);
    REQUIRE(result->slider_holds.size() == 1);
    const auto& hold = result->slider_holds.front();
    REQUIRE(hold.points.size() == 41);
    CHECK(hold.points.front().kind == xlair::sheets::SliderHoldPointKind::Start);
    CHECK(hold.points.back().kind == xlair::sheets::SliderHoldPointKind::End);
    for (std::size_t index = 1; index + 1 < hold.points.size(); ++index) {
        CHECK(hold.points[index].kind == xlair::sheets::SliderHoldPointKind::Invisible);
    }

    const auto& midpoint = hold.points[20];
    CHECK(midpoint.sample == 100);
    CHECK(midpoint.lane.start == 6);
    CHECK(midpoint.lane.width == 3);

    REQUIRE(hold.judge_points.size() == 9);
    CHECK(hold.judge_points[4].sample == 100);
    CHECK(hold.judge_points[4].lane.start == 6);
    CHECK(hold.judge_points[4].lane.width == 3);
}

TEST_CASE(
    "Compile supports multiple Control points in one Slider Hold Bezier segment", "[SheetsAnalyzer][SUS][Compiler]"
) {
    sus::Document document;
    document.slider_hold_points = {
        {
            .kind = sus::SliderHoldPointKind::Start,
            .lane = { .start = 0, .width = 2 },
            .channel = 1,
        },
        {
            .kind = sus::SliderHoldPointKind::Control,
            .position = { .measure = 0, .numerator = 1, .denominator = 4 },
            .lane = { .start = 13, .width = 2 },
            .channel = 1,
        },
        {
            .kind = sus::SliderHoldPointKind::Control,
            .position = { .measure = 0, .numerator = 3, .denominator = 4 },
            .lane = { .start = 13, .width = 2 },
            .channel = 1,
        },
        {
            .kind = sus::SliderHoldPointKind::End,
            .position = { .measure = 1 },
            .lane = { .start = 0, .width = 2 },
            .channel = 1,
        },
    };

    const auto result = sus::Compile(document, { .sample_rate = 100 });

    REQUIRE(result);
    const auto& midpoint = result->slider_holds.front().points[20];
    CHECK(midpoint.sample == 100);
    CHECK(midpoint.lane.start == 9);
    CHECK(midpoint.lane.width == 3);
}

TEST_CASE(
    "Compile generates Slider Hold judgements independently from rendering points", "[SheetsAnalyzer][SUS][Compiler]"
) {
    sus::Document document;
    document.slider_hold_points = {
        {
            .kind = sus::SliderHoldPointKind::Start,
            .lane = { .start = 0, .width = 2 },
            .channel = 1,
        },
        {
            .kind = sus::SliderHoldPointKind::Visible,
            .position = { .measure = 0, .numerator = 1, .denominator = 3 },
            .lane = { .start = 4, .width = 2 },
            .channel = 1,
        },
        {
            .kind = sus::SliderHoldPointKind::End,
            .position = { .measure = 1 },
            .lane = { .start = 8, .width = 2 },
            .channel = 1,
        },
    };

    const auto result = sus::Compile(document, { .sample_rate = 100 });

    REQUIRE(result);
    const auto& hold = result->slider_holds.front();
    CHECK(hold.points.size() == 3);
    REQUIRE(hold.judge_points.size() == 10);
    const auto visible_judgement =
        std::find_if(hold.judge_points.begin(), hold.judge_points.end(), [](const auto& point) {
            return point.sample == 67;
        });
    REQUIRE(visible_judgement != hold.judge_points.end());
    CHECK(visible_judgement->lane.start == 4);
    CHECK(visible_judgement->lane.width == 2);
    CHECK(result->total_combo == hold.judge_points.size());
}

TEST_CASE(
    "Compile keeps periodic Slider Hold judgements in musical time across BPM changes",
    "[SheetsAnalyzer][SUS][Compiler]"
) {
    sus::Document document;
    document.bpm_definitions[1] = 180.0;
    document.bpm_changes.push_back(
        {
            .position = { .measure = 0, .numerator = 1, .denominator = 2 },
            .definition = 1,
        }
    );
    document.slider_hold_points = {
        {
            .kind = sus::SliderHoldPointKind::Start,
            .lane = { .start = 0, .width = 2 },
            .channel = 1,
        },
        {
            .kind = sus::SliderHoldPointKind::End,
            .position = { .measure = 1 },
            .lane = { .start = 0, .width = 2 },
            .channel = 1,
        },
    };

    const auto result = sus::Compile(document, { .sample_rate = 120 });

    REQUIRE(result);
    const auto& judgements = result->slider_holds.front().judge_points;
    const s3d::Array<s3d::int64> expected_samples = { 0, 30, 60, 90, 120, 140, 160, 180, 200 };
    REQUIRE(judgements.size() == expected_samples.size());
    for (std::size_t index = 0; index < judgements.size(); ++index) {
        CHECK(judgements[index].sample == expected_samples[index]);
    }
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
        CHECK(result.diagnostics.front().message == U"A Slider Hold channel starts before its previous hold ends.");
    }

    SECTION("intermediate point without Start") {
        sus::Document document;
        document.slider_hold_points.push_back({ .kind = sus::SliderHoldPointKind::Visible, .channel = 1 });

        const auto result = sus::Compile(document, {});

        REQUIRE_FALSE(result);
        CHECK(result.diagnostics.front().message == U"A Slider Hold point has no active Start point on its channel.");
    }

    SECTION("End without Start") {
        sus::Document document;
        document.slider_hold_points.push_back({ .kind = sus::SliderHoldPointKind::End, .channel = 1 });

        const auto result = sus::Compile(document, {});

        REQUIRE_FALSE(result);
        CHECK(
            result.diagnostics.front().message == U"A Slider Hold End point has no active Start point on its channel."
        );
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
        document.slider_hold_points.push_back(
            {
                .kind = sus::SliderHoldPointKind::Start,
                .channel = 1,
                .timeline = 99,
            }
        );

        const auto result = sus::Compile(document, {});

        REQUIRE_FALSE(result);
        CHECK(result.diagnostics.front().message == U"A Slider Hold point references an undefined hispeed definition.");
    }

    SECTION("anchors at the same time") {
        sus::Document document;
        document.slider_hold_points = {
            { .kind = sus::SliderHoldPointKind::Start, .lane = { .start = 0 }, .channel = 1 },
            { .kind = sus::SliderHoldPointKind::End, .lane = { .start = 0 }, .channel = 1 },
        };

        const auto result = sus::Compile(document, {});

        REQUIRE_FALSE(result);
        CHECK(
            result.diagnostics.front().message ==
            U"Slider Hold anchors must be placed in strictly increasing time order."
        );
    }

    SECTION("point outside the slider") {
        sus::Document document;
        document.slider_hold_points = {
            { .kind = sus::SliderHoldPointKind::Start, .lane = { .start = 15, .width = 2 }, .channel = 1 },
            {
                .kind = sus::SliderHoldPointKind::End,
                .position = { .measure = 1 },
                .lane = { .start = 15, .width = 1 },
                .channel = 1,
            },
        };

        const auto result = sus::Compile(document, {});

        REQUIRE_FALSE(result);
        CHECK(
            result.diagnostics.front().message ==
            U"Slider Hold points must have a positive width and fit within XLAIR's 16 slider lanes."
        );
    }
}
