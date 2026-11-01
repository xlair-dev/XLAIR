#include <Playfield/ChartProjection.hpp>
#include <ThirdParty/Catch2/catch.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <random>
#include <utility>

namespace {
    namespace sheets = xlair::sheets;
    namespace playfield = xlair::playfield;

    sheets::Chart
    SingleSpanChart(const double speed = 1.0, const s3d::int64 before = 1000, const s3d::int64 after = 2000) {
        return {
            .sample_rate = 1000,
            .timelines = { { .speed_changes = { { .sample = 0, .multiplier = speed } } } },
            .slider_holds = {
                { .points = {
                    { .kind = sheets::SliderHoldPointKind::Start, .sample = before, .lane = { 2, 4 } },
                    { .kind = sheets::SliderHoldPointKind::End, .sample = after, .lane = { 7, 3 } },
                } },
            },
            .side_holds = {
                { .points = {
                    { .kind = sheets::SideHoldPointKind::Start, .sample = before },
                    { .kind = sheets::SideHoldPointKind::End, .sample = after },
                } },
            },
        };
    }

    void CheckPoint(const playfield::PreparedHoldPoint& actual, const playfield::PreparedHoldPoint& expected) {
        CHECK(actual.sample == expected.sample);
        CHECK(actual.timeline == expected.timeline);
        CHECK(actual.position == expected.position);
        CHECK(actual.lane_start == expected.lane_start);
        CHECK(actual.lane_width == expected.lane_width);
        CHECK(actual.color_progress == expected.color_progress);
    }

    void CheckSections(
        const s3d::Array<playfield::HoldSection>& actual,
        const s3d::Array<playfield::HoldSection>& expected
    ) {
        REQUIRE(actual.size() == expected.size());
        for (std::size_t index = 0; index < expected.size(); ++index) {
            CheckPoint(actual[index].before, expected[index].before);
            CheckPoint(actual[index].after, expected[index].after);
            CHECK(actual[index].before_distance == expected[index].before_distance);
            CHECK(actual[index].after_distance == expected[index].after_distance);
        }
    }

    bool Intersects(const playfield::HoldSection& section, const long double minimum, const long double maximum) {
        return std::max(section.before_distance, section.after_distance) >= minimum &&
               std::min(section.before_distance, section.after_distance) <= maximum;
    }

    bool CheckVisibleSections(
        const playfield::ChartProjection::Frame& frame,
        const playfield::PreparedHoldSpan& span,
        const long double minimum,
        const long double maximum
    ) {
        s3d::Array<playfield::HoldSection> unculled;
        s3d::Array<playfield::HoldSection> actual;
        frame.holdSections(span, unculled);
        frame.holdSections(span, actual, minimum, maximum);
        const bool visible = unculled.any([&](const auto& section) {
            return Intersects(section, minimum, maximum);
        });
        if (visible || !actual.isEmpty()) {
            CheckSections(actual, unculled);
        }
        return visible;
    }
}

TEST_CASE(
    "Hold visibility includes exact range boundaries and excludes only strict separation",
    "[Playfield][HoldVisibility]"
) {
    const playfield::ChartProjection projection{ SingleSpanChart() };
    s3d::Array<long double> positions;
    const auto frame = projection.frameAt(0, positions);
    s3d::Array<playfield::HoldSection> sections;
    for (const bool side : { false, true }) {
        const auto& span = side ? frame.sideHoldSpans(0).front() : frame.sliderHoldSpans(0).front();
        CAPTURE(side);
        REQUIRE(span.breakpoints.isEmpty());
        for (const auto [minimum, maximum] : {
                 std::pair{ 0.5L, 1.0L },
                 std::pair{ 2.0L, 3.0L },
                 std::pair{ 1.25L, 1.75L },
             }) {
            frame.holdSections(span, sections, minimum, maximum);
            REQUIRE(sections.size() == 1);
            CHECK(sections.front().before_distance == 1.0L);
            CHECK(sections.front().after_distance == 2.0L);
        }
        frame.holdSections(span, sections, 0.0L, std::nextafter(1.0L, 0.0L));
        CHECK(sections.isEmpty());
        frame.holdSections(span, sections, std::nextafter(2.0L, 3.0L), 3.0L);
        CHECK(sections.isEmpty());
    }
}

TEST_CASE("Hold visibility handles stopped and reverse intervals", "[Playfield][HoldVisibility]") {
    for (const auto speed : { 0.0, -1.0 }) {
        const playfield::ChartProjection projection{ SingleSpanChart(speed) };
        s3d::Array<long double> positions;
        const auto frame = projection.frameAt(0, positions);
        s3d::Array<playfield::HoldSection> sections;
        for (const bool side : { false, true }) {
            const auto& span = side ? frame.sideHoldSpans(0).front() : frame.sliderHoldSpans(0).front();
            CAPTURE(speed, side);
            if (speed == 0.0) {
                frame.holdSections(span, sections, 0.0L, 0.0L);
                REQUIRE(sections.size() == 1);
                CHECK(sections.front().before_distance == 0.0L);
                CHECK(sections.front().after_distance == 0.0L);
                frame.holdSections(span, sections, 0.1L, 1.0L);
            } else {
                frame.holdSections(span, sections, -1.0L, -1.0L);
                REQUIRE(sections.size() == 1);
                CHECK(sections.front().before_distance == -1.0L);
                CHECK(sections.front().after_distance == -2.0L);
                frame.holdSections(span, sections, -0.5L, 0.5L);
            }
            CHECK(sections.isEmpty());
        }
    }
}

TEST_CASE(
    "A reverse hold crossing the judgement clock remains visible between negative endpoints",
    "[Playfield][HoldVisibility]"
) {
    const playfield::ChartProjection projection{ SingleSpanChart(-1.0, 0, 1000) };
    s3d::Array<long double> positions;
    const auto frame = projection.frameAt(500, positions);
    s3d::Array<playfield::HoldSection> sections;
    for (const bool side : { false, true }) {
        const auto& span = side ? frame.sideHoldSpans(0).front() : frame.sliderHoldSpans(0).front();
        CAPTURE(side);
        frame.holdSections(span, sections, -0.1L, 0.1L);
        REQUIRE(sections.size() == 2);
        CHECK(sections.front().before_distance == -0.5L);
        CHECK(sections.front().after_distance == 0.0L);
        CHECK(sections.back().before_distance == 0.0L);
        CHECK(sections.back().after_distance == -0.5L);
        CHECK(CheckVisibleSections(frame, span, -0.1L, 0.1L));
        frame.holdSections(span, sections, -0.75L, -0.6L);
        CHECK(sections.isEmpty());
    }
}

TEST_CASE("Future speed turns are not culled using their endpoints alone", "[Playfield][HoldVisibility]") {
    auto chart = SingleSpanChart(1.0, 100, 1100);
    chart.timelines.front().speed_changes = {
        { .sample = 0, .multiplier = 1.0 },
        { .sample = 400, .multiplier = -2.0 },
        { .sample = 800, .multiplier = 2.0 },
    };
    const playfield::ChartProjection projection{ chart };
    s3d::Array<long double> positions;
    const auto frame = projection.frameAt(0, positions);
    s3d::Array<playfield::HoldSection> sections;
    for (const bool side : { false, true }) {
        const auto& span = side ? frame.sideHoldSpans(0).front() : frame.sliderHoldSpans(0).front();
        CAPTURE(side);
        REQUIRE(span.breakpoints.size() == 2);
        frame.holdSections(span, sections, -0.05L, 0.05L);
        REQUIRE(sections.size() == 3);
        CHECK(sections.front().before_distance > 0.05L);
        CHECK(sections.back().after_distance > 0.05L);
        CHECK(CheckVisibleSections(frame, span, -0.05L, 0.05L));
    }
}

TEST_CASE("Passed speed turns no longer prevent safely culling the remaining hold", "[Playfield][HoldVisibility]") {
    auto chart = SingleSpanChart(1.0, 100, 1100);
    chart.timelines.front().speed_changes = {
        { .sample = 0, .multiplier = 1.0 },
        { .sample = 400, .multiplier = -2.0 },
        { .sample = 800, .multiplier = 2.0 },
    };
    const playfield::ChartProjection projection{ chart };
    s3d::Array<long double> positions;
    s3d::Array<playfield::HoldSection> sections;
    for (const auto current : { 800, 900, 1200 }) {
        const auto frame = projection.frameAt(current, positions);
        for (const bool side : { false, true }) {
            const auto& span = side ? frame.sideHoldSpans(0).front() : frame.sliderHoldSpans(0).front();
            CAPTURE(current, side);
            REQUIRE_FALSE(span.breakpoints.isEmpty());
            CHECK(span.breakpoints.back().sample <= current);
            frame.holdSections(span, sections, 1.0L, 2.0L);
            CHECK(sections.isEmpty());
            CHECK(CheckVisibleSections(frame, span, -0.4L, 0.1L));
        }
    }
}

TEST_CASE("Hold culling uses the ending anchor's own timeline", "[Playfield][HoldVisibility]") {
    auto chart = SingleSpanChart();
    chart.timelines.push_back({ .speed_changes = { { .sample = 0, .multiplier = -1.0 } } });
    chart.slider_holds.front().points.back().timeline = 1;
    chart.side_holds.front().points.back().timeline = 1;
    const playfield::ChartProjection projection{ chart };
    s3d::Array<long double> positions;
    const auto frame = projection.frameAt(0, positions);
    s3d::Array<playfield::HoldSection> sections;
    for (const bool side : { false, true }) {
        const auto& span = side ? frame.sideHoldSpans(0).front() : frame.sliderHoldSpans(0).front();
        CAPTURE(side);
        frame.holdSections(span, sections, -0.1L, 0.1L);
        REQUIRE(sections.size() == 1);
        CHECK(sections.front().before_distance == 1.0L);
        CHECK(sections.front().after_distance == -2.0L);
        CHECK(CheckVisibleSections(frame, span, -0.1L, 0.1L));
        frame.holdSections(span, sections, 2.0L, 3.0L);
        CHECK(sections.isEmpty());
    }
}

TEST_CASE("Past holds use normal-speed distances when culling", "[Playfield][HoldVisibility]") {
    const playfield::ChartProjection projection{ SingleSpanChart(-3.0) };
    s3d::Array<long double> positions;
    const auto frame = projection.frameAt(2500, positions);
    s3d::Array<playfield::HoldSection> sections;
    const auto& span = frame.sliderHoldSpans(0).front();
    frame.holdSections(span, sections, -0.75L, -0.25L);
    REQUIRE(sections.size() == 1);
    CHECK(sections.front().before_distance == -1.5L);
    CHECK(sections.front().after_distance == -0.5L);
    frame.holdSections(span, sections, 0.1L, 1.0L);
    CHECK(sections.isEmpty());
}

TEST_CASE("Culled hold output clears previous sections without reallocating", "[Playfield][HoldVisibility]") {
    const playfield::ChartProjection projection{ SingleSpanChart() };
    s3d::Array<long double> positions;
    const auto frame = projection.frameAt(0, positions);
    const auto& span = frame.sliderHoldSpans(0).front();
    s3d::Array<playfield::HoldSection> expected;
    frame.holdSections(span, expected);
    s3d::Array<playfield::HoldSection> sections{ {} };
    sections.reserve(16);
    const auto capacity = sections.capacity();
    for (int iteration = 0; iteration < 4; ++iteration) {
        frame.holdSections(span, sections, 0.5L, 2.5L);
        CheckSections(sections, expected);
        CHECK(sections.capacity() == capacity);
        frame.holdSections(span, sections, 3.0L, 4.0L);
        CHECK(sections.isEmpty());
        CHECK(sections.capacity() == capacity);
    }
}

TEST_CASE(
    "Randomized hold culling never drops sections intersecting the requested range",
    "[Playfield][HoldVisibility]"
) {
    std::mt19937 random{ 0x584C4149 };
    const auto integer = [&](const int minimum, const int maximum) {
        return std::uniform_int_distribution<int>{ minimum, maximum }(random);
    };
    constexpr std::array speeds{ -3.0, -1.0, 0.0, 0.25, 1.0, 2.0, 5.0 };
    std::size_t visible_queries = 0;
    std::size_t invisible_queries = 0;
    for (int chart_index = 0; chart_index < 12; ++chart_index) {
        sheets::Chart chart{ .sample_rate = 1000 };
        for (int timeline = 0; timeline < 3; ++timeline) {
            sheets::Timeline source;
            for (const auto sample : { -500, 0, 200, 450, 900, 1300, 1800 }) {
                source.speed_changes.push_back({ .sample = sample, .multiplier = speeds[integer(0, 6)] });
            }
            chart.timelines.push_back(std::move(source));
        }
        for (int hold_index = 0; hold_index < 3; ++hold_index) {
            sheets::SliderHold slider;
            sheets::SideHold side;
            s3d::int64 sample = integer(-600, 100);
            for (int point_index = 0; point_index < 4; ++point_index) {
                const auto timeline = static_cast<sheets::TimelineIndex>(integer(0, 2));
                slider.points.push_back(
                    {
                        .timeline = timeline,
                        .sample = sample,
                        .lane = { .start = static_cast<s3d::uint8>(integer(0, 12)), .width = 4 },
                    }
                );
                side.points.push_back({ .timeline = timeline, .sample = sample });
                sample += integer(100, 900);
            }
            chart.slider_holds.push_back(std::move(slider));
            chart.side_holds.push_back(std::move(side));
        }
        const playfield::ChartProjection projection{ chart };
        s3d::Array<long double> positions;
        for (int query = 0; query < 32; ++query) {
            const s3d::int64 current = integer(-1000, 3500);
            const auto frame = projection.frameAt(current, positions);
            for (const bool side : { false, true }) {
                for (std::size_t hold_index = 0; hold_index < 3; ++hold_index) {
                    const auto spans = side ? frame.sideHoldSpans(hold_index) : frame.sliderHoldSpans(hold_index);
                    for (std::size_t span_index = 0; span_index < spans.size(); ++span_index) {
                        const long double minimum = integer(-50, 50) / 10.0L;
                        const long double maximum = minimum + integer(0, 30) / 10.0L;
                        CAPTURE(chart_index, query, current, side, hold_index, span_index, minimum, maximum);
                        if (CheckVisibleSections(frame, spans[span_index], minimum, maximum)) {
                            ++visible_queries;
                        } else {
                            ++invisible_queries;
                        }
                    }
                }
            }
        }
    }
    CHECK(visible_queries > 0);
    CHECK(invisible_queries > 0);
}
