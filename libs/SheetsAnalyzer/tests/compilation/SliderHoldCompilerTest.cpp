#include <ThirdParty/Catch2/catch.hpp>

#include "compilation/SliderHoldCompiler.hpp"

#include <algorithm>

namespace compilation = xlair::sheets::compilation;

TEST_CASE(
    "Compile expands Slider Hold Control points into generated Invisible points",
    "[SheetsAnalyzer][Compilation]"
) {
    s3d::Array<compilation::SliderPathPoint> points;
    points = {
        {
            .kind = compilation::SliderPathPoint::Kind::Start,
            .lane = { .start = 0, .width = 2 },
        },
        {
            .kind = compilation::SliderPathPoint::Kind::Control,
            .sample = 100,
            .lane = { .start = 13, .width = 2 },
        },
        {
            .kind = compilation::SliderPathPoint::Kind::End,
            .sample = 200,
            .lane = { .start = 0, .width = 2 },
        },
    };

    const auto result = compilation::detail::CompileSliderHold(points, 100, { { .sample = 0, .bpm = 120.0 } });

    REQUIRE(result);
    const auto& hold = (*result);
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
    "Compile supports multiple Control points in one Slider Hold Bezier segment",
    "[SheetsAnalyzer][Compilation]"
) {
    s3d::Array<compilation::SliderPathPoint> points;
    points = {
        {
            .kind = compilation::SliderPathPoint::Kind::Start,
            .lane = { .start = 0, .width = 2 },
        },
        {
            .kind = compilation::SliderPathPoint::Kind::Control,
            .sample = 50,
            .lane = { .start = 13, .width = 2 },
        },
        {
            .kind = compilation::SliderPathPoint::Kind::Control,
            .sample = 150,
            .lane = { .start = 13, .width = 2 },
        },
        {
            .kind = compilation::SliderPathPoint::Kind::End,
            .sample = 200,
            .lane = { .start = 0, .width = 2 },
        },
    };

    const auto result = compilation::detail::CompileSliderHold(points, 100, { { .sample = 0, .bpm = 120.0 } });

    REQUIRE(result);
    const auto& midpoint = (*result).points[20];
    CHECK(midpoint.sample == 100);
    CHECK(midpoint.lane.start == 9);
    CHECK(midpoint.lane.width == 3);
}

TEST_CASE(
    "Compile generates Slider Hold judgements independently from rendering points",
    "[SheetsAnalyzer][Compilation]"
) {
    s3d::Array<compilation::SliderPathPoint> points;
    points = {
        {
            .kind = compilation::SliderPathPoint::Kind::Start,
            .lane = { .start = 0, .width = 2 },
        },
        {
            .kind = compilation::SliderPathPoint::Kind::Visible,
            .sample = 67,
            .lane = { .start = 4, .width = 2 },
        },
        {
            .kind = compilation::SliderPathPoint::Kind::End,
            .sample = 200,
            .lane = { .start = 8, .width = 2 },
        },
    };

    const auto result = compilation::detail::CompileSliderHold(points, 100, { { .sample = 0, .bpm = 120.0 } });

    REQUIRE(result);
    const auto& hold = (*result);
    CHECK(hold.points.size() == 3);
    REQUIRE(hold.judge_points.size() == 10);
    const auto visible_judgement =
        std::find_if(hold.judge_points.begin(), hold.judge_points.end(), [](const auto& point) {
            return point.sample == 67;
        });
    REQUIRE(visible_judgement != hold.judge_points.end());
    CHECK(visible_judgement->lane.start == 4);
    CHECK(visible_judgement->lane.width == 2);
}

TEST_CASE(
    "Compile keeps periodic Slider Hold judgements in musical time across BPM changes",
    "[SheetsAnalyzer][Compilation]"
) {
    s3d::Array<compilation::SliderPathPoint> points;
    points = {
        {
            .kind = compilation::SliderPathPoint::Kind::Start,
            .lane = { .start = 0, .width = 2 },
        },
        {
            .kind = compilation::SliderPathPoint::Kind::End,
            .sample = 200,
            .lane = { .start = 0, .width = 2 },
        },
    };

    const auto result = compilation::detail::CompileSliderHold(
        points,
        120,
        { { .sample = 0, .bpm = 120.0 }, { .sample = 120, .bpm = 180.0 } }
    );

    REQUIRE(result);
    const auto& judgements = (*result).judge_points;
    const s3d::Array<s3d::int64> expected_samples = { 0, 30, 60, 90, 120, 140, 160, 180, 200 };
    REQUIRE(judgements.size() == expected_samples.size());
    for (std::size_t index = 0; index < judgements.size(); ++index) {
        CHECK(judgements[index].sample == expected_samples[index]);
    }
}
