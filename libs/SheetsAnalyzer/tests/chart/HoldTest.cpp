#include <ThirdParty/Catch2/catch.hpp>

#include "chart/HoldJudgement.hpp"
#include "chart/SliderHoldCompiler.hpp"

#include <SheetsAnalyzer/SliderHoldPath.hpp>

#include <algorithm>

namespace sheets = xlair::sheets;
namespace compiler = sheets::chart;

TEST_CASE("Normalized hold paths retain their points without resampling or extra combo", "[SheetsAnalyzer][IR][Hold]") {
    const auto hold = compiler::detail::CompileSliderHold(
        {
            { .kind = sheets::SliderHoldPointKind::Start, .sample = 0, .lane = { 0, 2 } },
            { .kind = sheets::SliderHoldPointKind::Invisible, .sample = 50, .lane = { 13, 2 } },
            { .kind = sheets::SliderHoldPointKind::Invisible, .sample = 150, .lane = { 13, 2 } },
            { .kind = sheets::SliderHoldPointKind::End, .sample = 200, .lane = { 0, 2 } },
        },
        100,
        { { .sample = 0, .bpm = 120.0 } }
    );
    REQUIRE(hold);
    REQUIRE(hold->points.size() == 4);
    REQUIRE(hold->judge_points.size() == 9);
    CHECK(hold->points[1].sample == 50);
    CHECK(hold->points[1].lane.start == 13);
    CHECK(hold->points[1].lane.width == 2);
    CHECK(hold->judge_points[1].lane.start == 6);
    CHECK(hold->judge_points[1].lane.width == 3);
    CHECK(hold->judge_points[4].lane.start == 13);
    CHECK(hold->judge_points[4].lane.width == 2);
    for (const auto& point : hold->judge_points) {
        const auto lane = sheets::EvaluateSliderHoldLane(hold->points, point.sample);
        REQUIRE(lane.has_value());
        CHECK(lane->start == point.lane.start);
        CHECK(lane->width == point.lane.width);
    }
}

TEST_CASE("Off-grid visible anchors get a judgement independently of rendering density", "[SheetsAnalyzer][IR][Hold]") {
    const auto hold = compiler::detail::CompileSliderHold(
        {
            { .kind = sheets::SliderHoldPointKind::Start, .sample = 0, .lane = { 0, 2 } },
            { .kind = sheets::SliderHoldPointKind::Visible, .timeline = 1, .sample = 67, .lane = { 4, 2 } },
            { .kind = sheets::SliderHoldPointKind::End, .sample = 200, .lane = { 8, 2 } },
        },
        100,
        { { .sample = 0, .bpm = 120.0 } }
    );
    REQUIRE(hold);
    CHECK(hold->points.size() == 3);
    REQUIRE(hold->judge_points.size() == 10);
    const auto visible = std::find_if(hold->judge_points.begin(), hold->judge_points.end(), [](const auto& point) {
        return point.sample == 67;
    });
    REQUIRE(visible != hold->judge_points.end());
    CHECK(visible->lane.start == 4);
    CHECK(visible->lane.width == 2);
    CHECK(hold->points[1].timeline == 1);
}

TEST_CASE("Hidden path points retain their timeline without adding checkpoints", "[SheetsAnalyzer][IR][Hold]") {
    const auto hold = compiler::detail::CompileSliderHold(
        {
            { .kind = sheets::SliderHoldPointKind::Start, .sample = 0, .lane = { 0, 2 } },
            { .kind = sheets::SliderHoldPointKind::Invisible, .timeline = 1, .sample = 67, .lane = { 4, 2 } },
            { .kind = sheets::SliderHoldPointKind::End, .sample = 200, .lane = { 8, 2 } },
        },
        100,
        { { .sample = 0, .bpm = 120.0 } }
    );
    REQUIRE(hold);
    REQUIRE(hold->points.size() == 3);
    CHECK(hold->points[1].timeline == 1);
    CHECK(hold->judge_points.size() == 9);
    CHECK(std::none_of(hold->judge_points.begin(), hold->judge_points.end(), [](const auto& point) {
        return point.sample == 67;
    }));
}

TEST_CASE("Judgement lanes follow the last path point at an instantaneous jump", "[SheetsAnalyzer][IR][Hold]") {
    const auto hold = compiler::detail::CompileSliderHold(
        {
            { .kind = sheets::SliderHoldPointKind::Start, .sample = 0, .lane = { 0, 2 } },
            { .kind = sheets::SliderHoldPointKind::Invisible, .sample = 100, .lane = { 4, 2 } },
            { .kind = sheets::SliderHoldPointKind::Invisible, .sample = 100, .lane = { 8, 2 } },
            { .kind = sheets::SliderHoldPointKind::End, .sample = 200, .lane = { 12, 2 } },
        },
        100,
        { { .sample = 0, .bpm = 120.0 } }
    );
    REQUIRE(hold);
    REQUIRE(hold->points.size() == 4);
    REQUIRE(hold->judge_points.size() == 9);
    CHECK(hold->judge_points[4].sample == 100);
    CHECK(hold->judge_points[4].lane.start == 8);
    CHECK(hold->judge_points[4].lane.width == 2);
    CHECK(hold->judge_points[5].lane.start == 9);
}

TEST_CASE("Hold judgements follow BPM changes in musical time", "[SheetsAnalyzer][IR][Hold]") {
    const auto samples = compiler::detail::GenerateHoldJudgeSamples(
        0,
        200,
        120,
        { { .sample = 0, .bpm = 120.0 }, { .sample = 120, .bpm = 180.0 } }
    );
    REQUIRE(samples);
    const s3d::Array<s3d::int64> expected{ 0, 30, 60, 90, 120, 140, 160, 180, 200 };
    CHECK(*samples == expected);
    CHECK_FALSE(compiler::detail::GenerateHoldJudgeSamples(0, 1, 1, { { .sample = 0, .bpm = 1000.0 } }));
    CHECK_FALSE(compiler::detail::GenerateHoldJudgeSamples(0, 0, 120, { { .sample = 0, .bpm = 120.0 } }));
}

TEST_CASE("Runtime hold compilation validates quantized point positions", "[SheetsAnalyzer][IR][Hold]") {
    const s3d::Array<sheets::TempoChange> tempos{ { .sample = 0, .bpm = 120.0 } };
    CHECK_FALSE(
        compiler::detail::CompileSliderHold(
            {
                { .kind = sheets::SliderHoldPointKind::Start, .sample = 0, .lane = { 15, 2 } },
                { .kind = sheets::SliderHoldPointKind::End, .sample = 100, .lane = { 15, 1 } },
            },
            100,
            tempos
        )
    );
    CHECK_FALSE(
        compiler::detail::CompileSliderHold(
            {
                { .kind = sheets::SliderHoldPointKind::Start, .sample = 0, .lane = { 0, 2 } },
                { .kind = sheets::SliderHoldPointKind::End, .sample = 0, .lane = { 0, 2 } },
            },
            100,
            tempos
        )
    );
}
