#include <ThirdParty/Catch2/catch.hpp>

#include "core/judgement/JudgeEngine.hpp"

#include <array>

namespace judge = xlair::core::judgement;
namespace sheets = xlair::sheets;

TEST_CASE("Slider hold activity follows contact along its path", "[Game][Judgement]") {
    sheets::Chart chart;
    chart.sample_rate = 1000;
    chart.slider_holds = { {
        .points = {
            { .kind = sheets::SliderHoldPointKind::Start, .sample = 1000, .lane = { .start = 2 } },
            { .sample = 2000, .lane = { .start = 8 } },
            { .kind = sheets::SliderHoldPointKind::End, .sample = 3000, .lane = { .start = 2 } },
        },
        .judge_points = {
            { .sample = 1000, .lane = { .start = 2 } },
            { .sample = 2000, .lane = { .start = 8 } },
            { .sample = 3000, .lane = { .start = 2 } },
        },
    } };
    chart.total_combo = 3;
    judge::JudgeEngine engine{ chart };
    CHECK_FALSE(engine.sliderHoldActive(0));
    const std::array touches{ judge::TouchEvent{ .zone = 4, .sample = 1000 } };
    REQUIRE(engine.advance(1000, touches, {}, uint32{ 1 } << 4).size() == 1);
    CHECK(engine.sliderHoldActive(0));
    CHECK(engine.advance(1500, {}, {}, uint32{ 1 } << 10).isEmpty());
    CHECK(engine.sliderHoldActive(0));
    (void)engine.advance(1501, {}, {}, uint32{ 1 } << 4);
    CHECK_FALSE(engine.sliderHoldActive(0));
    (void)engine.advance(1502, {}, {}, uint32{ 1 } << 10);
    CHECK(engine.sliderHoldActive(0));
    (void)engine.advance(1503);
    CHECK_FALSE(engine.sliderHoldActive(0));
    REQUIRE(engine.advance(2000, {}, {}, uint32{ 1 } << 16).size() == 1);
    CHECK(engine.sliderHoldActive(0));
    REQUIRE(engine.advance(2934, {}, {}, uint32{ 1 } << 4).size() == 1);
    CHECK_FALSE(engine.sliderHoldActive(0));
}

TEST_CASE("Overlapping side holds retain independent activity", "[Game][Judgement]") {
    sheets::Chart chart;
    chart.sample_rate = 1000;
    chart.side_holds = { {
        .button = sheets::SideButton::LeftUpper,
        .points = {
            { .kind = sheets::SideHoldPointKind::Start, .sample = 1000 },
            { .kind = sheets::SideHoldPointKind::End, .sample = 3000 },
        },
        .judge_samples = { 1000, 2000, 3000 },
    } };
    chart.side_holds.push_back(chart.side_holds.front());
    chart.side_holds.back().button = sheets::SideButton::RightUpper;
    chart.total_combo = 6;
    judge::JudgeEngine engine{ chart };
    const std::array sides{
        judge::SideEvent{ .button = sheets::SideButton::LeftUpper, .sample = 1000 },
        judge::SideEvent{ .button = sheets::SideButton::RightUpper, .sample = 1000 },
    };
    REQUIRE(engine.advance(1000, {}, sides, 0, 3).size() == 2);
    CHECK(engine.sideHoldActive(0));
    CHECK(engine.sideHoldActive(1));
    (void)engine.advance(1500, {}, {}, 0, 2);
    CHECK_FALSE(engine.sideHoldActive(0));
    CHECK(engine.sideHoldActive(1));
    (void)engine.advance(1501);
    CHECK_FALSE(engine.sideHoldActive(1));
}

TEST_CASE("Side holds recover at checkpoints and stop on release or end", "[Game][Judgement]") {
    sheets::Chart chart;
    chart.sample_rate = 1000;
    chart.side_holds = { {
        .button = sheets::SideButton::LeftUpper,
        .points = {
            { .kind = sheets::SideHoldPointKind::Start, .sample = 1000 },
            { .kind = sheets::SideHoldPointKind::End, .sample = 4000 },
        },
        .judge_samples = { 1000, 2000, 3000, 4000 },
    } };
    chart.total_combo = 4;
    judge::JudgeEngine engine{ chart };
    REQUIRE(engine.advance(1200).front().grade == judge::Grade::Miss);
    CHECK_FALSE(engine.sideHoldActive(0));
    CHECK(engine.advance(1500, {}, {}, 0, 1).isEmpty());
    CHECK_FALSE(engine.sideHoldActive(0));
    const auto recovered = engine.advance(2000, {}, {}, 0, 1);
    REQUIRE(recovered.size() == 1);
    CHECK(recovered.front().grade == judge::Grade::Perfect);
    CHECK(engine.sideHoldActive(0));
    (void)engine.advance(2001);
    CHECK_FALSE(engine.sideHoldActive(0));
    (void)engine.advance(2002, {}, {}, 0, 2);
    CHECK_FALSE(engine.sideHoldActive(0));
    (void)engine.advance(2003, {}, {}, 0, 1);
    CHECK(engine.sideHoldActive(0));
    const auto missed = engine.advance(3200);
    REQUIRE(missed.size() == 1);
    CHECK(missed.front().grade == judge::Grade::Miss);
    CHECK_FALSE(engine.sideHoldActive(0));
    (void)engine.advance(3300, {}, {}, 0, 1);
    CHECK(engine.sideHoldActive(0));
    (void)engine.advance(4000, {}, {}, 0, 1);
    CHECK_FALSE(engine.sideHoldActive(0));
}

TEST_CASE("X-taps acquire their co-located hold starts with the same input", "[Game][Judgement]") {
    sheets::Chart chart;
    chart.sample_rate = 1000;
    chart.total_combo = 4;

    SECTION("slider") {
        chart.slider_notes = { { .kind = sheets::SliderNoteKind::XTap, .sample = 1000, .lane = { .start = 2 } } };
        chart.slider_holds = { {
            .points = {
                { .kind = sheets::SliderHoldPointKind::Start, .sample = 1000, .lane = { .start = 2 } },
                { .kind = sheets::SliderHoldPointKind::End, .sample = 3000, .lane = { .start = 2 } },
            },
            .judge_points = {
                { .sample = 1000, .lane = { .start = 2 } },
                { .sample = 2000, .lane = { .start = 2 } },
                { .sample = 3000, .lane = { .start = 2 } },
            },
        } };
        judge::JudgeEngine engine{ chart };
        const std::array touches{ judge::TouchEvent{ .zone = 4, .sample = 1080 } };
        const auto events = engine.advance(1080, touches, {}, uint32{ 1 } << 4);
        REQUIRE(events.size() == 2);
        CHECK(events[0].grade == judge::Grade::Perfect);
        CHECK(events[1].note_kind == judge::NoteKind::SliderHold);
        CHECK(events[1].linked_start);
        CHECK(engine.sliderHoldPointState(0, 0) == judge::NoteState::Hit);
        CHECK(engine.sliderHoldActive(0));
    }
    SECTION("side") {
        chart.side_notes = {
            { .sample = 1000, .button = sheets::SideButton::LeftUpper, .kind = sheets::SideNoteKind::XTap }
        };
        chart.side_holds = { {
            .button = sheets::SideButton::LeftUpper,
            .points = {
                { .kind = sheets::SideHoldPointKind::Start, .sample = 1000 },
                { .kind = sheets::SideHoldPointKind::End, .sample = 3000 },
            },
            .judge_samples = { 1000, 2000, 3000 },
        } };
        judge::JudgeEngine engine{ chart };
        const std::array sides{ judge::SideEvent{ .button = sheets::SideButton::LeftUpper, .sample = 1080 } };
        const auto events = engine.advance(1080, {}, sides, 0, 1);
        REQUIRE(events.size() == 2);
        CHECK(events[0].grade == judge::Grade::Perfect);
        CHECK(events[1].note_kind == judge::NoteKind::SideHold);
        CHECK(events[1].linked_start);
        CHECK(engine.sideHoldPointState(0, 0) == judge::NoteState::Hit);
        CHECK(engine.sideHoldActive(0));
    }
}
