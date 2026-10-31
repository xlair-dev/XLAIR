#include <ThirdParty/Catch2/catch.hpp>

#include "chart/HoldJudgement.hpp"
#include "chart/SliderHoldCompiler.hpp"

#include <SheetsAnalyzer/SliderHoldPath.hpp>

#include <algorithm>
#include <cmath>

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

TEST_CASE(
    "Visible anchors restart periodic judgements independently of rendering density",
    "[SheetsAnalyzer][IR][Hold]"
) {
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
    const s3d::Array<s3d::int64> expected{ 0, 25, 50, 67, 92, 117, 142, 167, 192, 200 };
    for (std::size_t index = 0; index < expected.size(); ++index) {
        CHECK(hold->judge_points[index].sample == expected[index]);
    }
}

TEST_CASE("Hold judgement density follows BPM thresholds", "[SheetsAnalyzer][IR][Hold]") {
    const auto bpm = GENERATE(60.0, 119.0, 120.0, 239.0, 240.0, 480.0);
    const auto samples = compiler::detail::GenerateHoldJudgeSamples(100, 2000, 480, { { .sample = 0, .bpm = bpm } });
    REQUIRE(samples);
    REQUIRE(samples->size() > 2);
    const double beats = bpm < 120.0 ? 0.25 : (bpm < 240.0 ? 0.5 : 1.0);
    CHECK((*samples)[1] == 100 + static_cast<s3d::int64>(std::round(480.0 * 60.0 / bpm * beats)));
    CHECK(samples->back() == 2000);
}

TEST_CASE("Staggered holds align their periodic judgements at a shared visible anchor", "[SheetsAnalyzer][IR][Hold]") {
    const s3d::Array<sheets::TempoChange> tempos{ { .sample = 0, .bpm = 120.0 } };
    const s3d::Array<s3d::int64> anchors{ 167, 167, 192 };
    const auto first = compiler::detail::GenerateHoldJudgeSamples(0, 300, 100, tempos, anchors);
    const auto second = compiler::detail::GenerateHoldJudgeSamples(12, 300, 100, tempos, anchors);
    REQUIRE(first);
    REQUIRE(second);
    CHECK((*first)[1] == 25);
    CHECK((*second)[1] == 37);
    const s3d::Array<s3d::int64> expected_tail{ 167, 192, 217, 242, 267, 292, 300 };
    const auto first_anchor = std::find(first->begin(), first->end(), 167);
    const auto second_anchor = std::find(second->begin(), second->end(), 167);
    CHECK(s3d::Array<s3d::int64>(first_anchor, first->end()) == expected_tail);
    CHECK(s3d::Array<s3d::int64>(second_anchor, second->end()) == expected_tail);
    CHECK_FALSE(compiler::detail::GenerateHoldJudgeSamples(0, 300, 100, tempos, s3d::Array<s3d::int64>{ 200, 100 }));
    CHECK_FALSE(compiler::detail::GenerateHoldJudgeSamples(0, 300, 100, tempos, s3d::Array<s3d::int64>{ 301 }));
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
