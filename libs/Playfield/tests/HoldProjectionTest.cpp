#include <Playfield/ChartProjection.hpp>
#include <Playfield/HoldProjection.hpp>
#include <ThirdParty/Catch2/catch.hpp>

#include <array>
#include <limits>
#include <stdexcept>

namespace {
    namespace sheets = xlair::sheets;
    namespace playfield = xlair::playfield;

    struct LegacyPoint {
        s3d::int64 sample = 0;
        sheets::TimelineIndex timeline = 0;
        double lane_start = 0.0;
        double lane_width = 1.0;
        double color_progress = 0.0;
    };

    struct LegacySection {
        LegacyPoint before;
        LegacyPoint after;
        long double before_distance = 0.0L;
        long double after_distance = 0.0L;
    };

    template <class Point>
    LegacyPoint LegacyPointAt(
        const Point& before,
        const Point& after,
        const s3d::int64 sample,
        const s3d::int64 first_sample,
        const s3d::int64 last_sample
    ) {
        LegacyPoint point{ .sample = sample, .timeline = sample == after.sample ? after.timeline : before.timeline };
        if constexpr (requires { before.lane; }) {
            const double ratio = static_cast<double>(
                (static_cast<long double>(sample) - before.sample) /
                (static_cast<long double>(after.sample) - before.sample)
            );
            point.lane_start = s3d::Math::Lerp(
                static_cast<double>(before.lane.start),
                static_cast<double>(after.lane.start),
                ratio
            );
            point.lane_width = s3d::Math::Lerp(
                static_cast<double>(before.lane.width),
                static_cast<double>(after.lane.width),
                ratio
            );
            const double duration = static_cast<double>(last_sample) - static_cast<double>(first_sample);
            point.color_progress = duration > 0.0
                                       ? s3d::Clamp((static_cast<double>(sample) - first_sample) / duration, 0.0, 1.0)
                                       : 0.0;
        }
        return point;
    }

    // Keep the old renderer's raw-anchor traversal independent of the prepared cache.
    template <class Point>
    s3d::Array<LegacySection> LegacySections(
        const playfield::ChartProjection& projection,
        const s3d::Array<Point>& points,
        const std::size_t index,
        const s3d::int64 current
    ) {
        s3d::Array<LegacySection> sections;
        const auto& before = points[index];
        const auto& after = points[index + 1];
        if (before.sample >= after.sample) {
            return sections;
        }
        auto samples = projection.noteDistanceBreakpoints(before.timeline, current, before.sample, after.sample);
        samples.push_back(after.sample);
        auto previous = LegacyPointAt(before, after, before.sample, points.front().sample, points.back().sample);
        for (const auto sample : samples) {
            const auto next = LegacyPointAt(before, after, sample, points.front().sample, points.back().sample);
            sections.push_back({
                .before = previous,
                .after = next,
                .before_distance = projection.noteDistance(previous.timeline, current, previous.sample),
                .after_distance = projection.noteDistance(next.timeline, current, next.sample),
            });
            previous = next;
        }
        return sections;
    }

    constexpr double FieldFar = 129.2447;
    constexpr double FieldNear = -FieldFar * 100.0 / 3900.0;
    constexpr double FieldLength = FieldFar - FieldNear;
    constexpr double SourceLaneWidth = (1000.0 - 32.0 * 2.0) / 16.0;
    constexpr double WorldUnitsPerSecond = FieldLength * 100.0 / 880.0;

    double WorldZ(const long double distance, const double speed) {
        return static_cast<double>(distance) * WorldUnitsPerSecond * speed;
    }

    template <class Point>
    void CheckSliderPixels(
        const Point& actual,
        const LegacyPoint& expected,
        const long double actual_distance,
        const long double expected_distance
    ) {
        const double actual_left = 32.0 + actual.lane_start * SourceLaneWidth + 5.0;
        const double expected_left = 32.0 + expected.lane_start * SourceLaneWidth + 5.0;
        const double actual_width = actual.lane_width * SourceLaneWidth - 10.0;
        const double expected_width = expected.lane_width * SourceLaneWidth - 10.0;
        CHECK(actual_left == expected_left);
        CHECK(actual_width == expected_width);
        CHECK(actual_left + actual_width == expected_left + expected_width);
        CHECK((actual_left + (actual_left + actual_width)) * 0.5 ==
              (expected_left + (expected_left + expected_width)) * 0.5);
        for (const double speed : { 0.5, 1.0, 6.4 }) {
            CHECK(WorldZ(actual_distance, speed) == WorldZ(expected_distance, speed));
            CHECK((FieldFar - WorldZ(actual_distance, speed)) * 4000.0 / FieldLength ==
                  (FieldFar - WorldZ(expected_distance, speed)) * 4000.0 / FieldLength);
        }
        const auto start_color = s3d::ColorF{ U"#EC43F240" }.removeSRGBCurve();
        const auto end_color = s3d::ColorF{ U"#43A4F250" }.removeSRGBCurve();
        const auto actual_color = s3d::Math::Lerp(start_color, end_color, actual.color_progress);
        const auto expected_color = s3d::Math::Lerp(start_color, end_color, expected.color_progress);
        CHECK(actual_color.r == expected_color.r);
        CHECK(actual_color.g == expected_color.g);
        CHECK(actual_color.b == expected_color.b);
        CHECK(actual_color.a == expected_color.a);
    }

    void CheckSideGeometry(const playfield::HoldSection& actual, const LegacySection& expected) {
        for (const double speed : { 0.5, 1.0, 6.4 }) {
            const double actual_before = WorldZ(actual.before_distance, speed);
            const double actual_after = WorldZ(actual.after_distance, speed);
            const double expected_before = WorldZ(expected.before_distance, speed);
            const double expected_after = WorldZ(expected.after_distance, speed);
            for (const double near_limit : { 0.0, FieldNear, FieldNear - 8.0 }) {
                const auto actual_near = s3d::Clamp(s3d::Min(actual_before, actual_after), near_limit, FieldFar);
                const auto actual_far = s3d::Clamp(s3d::Max(actual_before, actual_after), near_limit, FieldFar);
                const auto expected_near = s3d::Clamp(s3d::Min(expected_before, expected_after), near_limit, FieldFar);
                const auto expected_far = s3d::Clamp(s3d::Max(expected_before, expected_after), near_limit, FieldFar);
                CHECK(actual_near == expected_near);
                CHECK(actual_far == expected_far);
                CHECK(actual_far - actual_near == expected_far - expected_near);
                CHECK((actual_far - actual_near < 0.01) == (expected_far - expected_near < 0.01));
                CHECK((actual_near + actual_far) * 0.5 == (expected_near + expected_far) * 0.5);
            }
        }
    }

    template <class Point>
    void CheckPoint(
        const playfield::ChartProjection& projection,
        const playfield::PreparedHoldPoint& actual,
        const LegacyPoint& expected,
        const long double actual_distance,
        const long double expected_distance
    ) {
        CHECK(actual.sample == expected.sample);
        CHECK(actual.timeline == expected.timeline);
        CHECK(actual.position == projection.positionAt(expected.timeline, expected.sample));
        CHECK(actual_distance == expected_distance);
        if constexpr (requires(Point point) { point.lane; }) {
            CHECK(actual.lane_start == expected.lane_start);
            CHECK(actual.lane_width == expected.lane_width);
            CHECK(actual.color_progress == expected.color_progress);
            CheckSliderPixels(actual, expected, actual_distance, expected_distance);
        }
    }

    template <class Point>
    void CheckSpans(
        const playfield::ChartProjection& projection,
        const playfield::ChartProjection::Frame& frame,
        const std::span<const playfield::PreparedHoldSpan> spans,
        const s3d::Array<Point>& points,
        const s3d::int64 current,
        s3d::Array<playfield::HoldSection>& sections
    ) {
        REQUIRE(spans.size() == (points.isEmpty() ? 0 : points.size() - 1));
        for (std::size_t index = 0; index < spans.size(); ++index) {
            CAPTURE(current, index);
            const auto expected = LegacySections(projection, points, index, current);
            frame.holdSections(spans[index], sections);
            REQUIRE(sections.size() == expected.size());
            if (points[index].sample >= points[index + 1].sample) {
                continue;
            }
            CHECK(spans[index].first_sample == points.front().sample);
            CHECK(spans[index].duration ==
                  static_cast<double>(points.back().sample) - static_cast<double>(points.front().sample));
            for (std::size_t section = 0; section < sections.size(); ++section) {
                CAPTURE(section);
                const auto& actual = sections[section];
                const auto& old = expected[section];
                CheckPoint<Point>(projection, actual.before, old.before, actual.before_distance, old.before_distance);
                CheckPoint<Point>(projection, actual.after, old.after, actual.after_distance, old.after_distance);
                if constexpr (!requires(Point point) { point.lane; }) {
                    CheckSideGeometry(actual, old);
                }
            }
        }
    }

    sheets::Chart ChartWithHolds() {
        return {
            .sample_rate = 44'100,
            .timelines = {
                {},
                { .speed_changes = {
                    { .sample = 200, .multiplier = 2.0 },
                    { .sample = -100, .multiplier = 1.25 },
                    { .sample = 0, .multiplier = 0.0 },
                    { .sample = 100, .multiplier = -1.5 },
                    { .sample = 200, .multiplier = 0.1 },
                    { .sample = 350, .multiplier = 2.0 },
                } },
                { .speed_changes = {
                    { .sample = 75, .multiplier = 1.0 / 3.0 },
                    { .sample = 220, .multiplier = 0.0 },
                    { .sample = 275, .multiplier = -2.5 },
                    { .sample = 450, .multiplier = 0.5 },
                } },
            },
            .slider_holds = {
                { .points = {
                    { .kind = sheets::SliderHoldPointKind::Start, .timeline = 1, .sample = -225, .lane = { 1, 3 } },
                    { .kind = sheets::SliderHoldPointKind::End, .timeline = 2, .sample = 600, .lane = { 10, 5 } },
                } },
                { .points = {
                    { .timeline = 0, .sample = -100, .lane = { 0, 2 } },
                    { .timeline = 1, .sample = 100, .lane = { 4, 3 } },
                    { .timeline = 2, .sample = 100, .lane = { 7, 1 } },
                    { .timeline = 0, .sample = 80, .lane = { 5, 2 } },
                    { .timeline = 2, .sample = 375, .lane = { 2, 8 } },
                    { .timeline = 1, .sample = 600, .lane = { 9, 2 } },
                } },
            },
            .side_holds = {
                { .button = sheets::SideButton::LeftUpper,
                  .points = {
                      { .timeline = 2, .sample = -225 },
                      { .timeline = 1, .sample = 600 },
                  } },
                { .button = sheets::SideButton::RightLower,
                  .points = {
                      { .timeline = 1, .sample = -100 },
                      { .timeline = 2, .sample = 100 },
                      { .timeline = 0, .sample = 100 },
                      { .timeline = 2, .sample = 80 },
                      { .timeline = 1, .sample = 375 },
                      { .timeline = 2, .sample = 600 },
                  } },
            },
        };
    }
}

TEST_CASE("Prepared holds exactly match raw-anchor rendering across seeks", "[Playfield][HoldProjection]") {
    const auto chart = ChartWithHolds();
    const playfield::ChartProjection projection{ chart };
    constexpr std::array<s3d::int64, 27> current_samples{
        -500, -225, -100, 0, 74, 75, 76, 99, 100, 101, 199, 200, 201, 219,
        220, 221, 275, 350, 374, 375, 450, 600, 900, 200, -100, 100, -500,
    };
    s3d::Array<long double> positions;
    s3d::Array<playfield::HoldSection> sections;
    positions.reserve(16);
    sections.reserve(64);
    const auto position_capacity = positions.capacity();
    const auto section_capacity = sections.capacity();
    for (const auto current : current_samples) {
        const auto frame = projection.frameAt(current, positions);
        for (std::size_t hold = 0; hold < chart.slider_holds.size(); ++hold) {
            CAPTURE(hold);
            CheckSpans(projection, frame, frame.sliderHoldSpans(hold), chart.slider_holds[hold].points, current, sections);
        }
        for (std::size_t hold = 0; hold < chart.side_holds.size(); ++hold) {
            CAPTURE(hold);
            CheckSpans(projection, frame, frame.sideHoldSpans(hold), chart.side_holds[hold].points, current, sections);
        }
        CHECK(positions.capacity() == position_capacity);
        CHECK(sections.capacity() == section_capacity);
    }
}

TEST_CASE("Prepared holds do not retain passed speed boundaries", "[Playfield][HoldProjection]") {
    const auto chart = ChartWithHolds();
    const playfield::ChartProjection projection{ chart };
    s3d::Array<long double> positions;
    s3d::Array<playfield::HoldSection> sections;
    const auto check_samples = [&](const s3d::int64 current, const s3d::Array<s3d::int64>& expected_ends) {
        const auto frame = projection.frameAt(current, positions);
        frame.holdSections(frame.sliderHoldSpans(0).front(), sections);
        REQUIRE(sections.size() == expected_ends.size());
        for (std::size_t index = 0; index < sections.size(); ++index) {
            CHECK(sections[index].after.sample == expected_ends[index]);
            CHECK(sections[index].after.timeline == (expected_ends[index] == 600 ? 2 : 1));
        }
    };
    check_samples(100, { 100, 200, 350, 600 });
    check_samples(250, { 250, 350, 600 });
    check_samples(350, { 350, 600 });
    check_samples(600, { 600 });
    check_samples(900, { 600 });
    check_samples(0, { 0, 100, 200, 350, 600 });
}

TEST_CASE("Prepared holds retain empty slots and reject invalid hold indices", "[Playfield][HoldProjection]") {
    auto chart = ChartWithHolds();
    chart.slider_holds.push_back({});
    chart.slider_holds.push_back({ .points = { { .sample = 50 } } });
    chart.side_holds.push_back({});
    chart.side_holds.push_back({ .points = { { .sample = 50 } } });
    const playfield::ChartProjection projection{ chart };
    s3d::Array<long double> positions;
    const auto frame = projection.frameAt(100, positions);
    CHECK(frame.sliderHoldSpans(2).empty());
    CHECK(frame.sliderHoldSpans(3).empty());
    CHECK(frame.sideHoldSpans(2).empty());
    CHECK(frame.sideHoldSpans(3).empty());
    CHECK_THROWS_AS(frame.sliderHoldSpans(4), std::out_of_range);
    CHECK_THROWS_AS(frame.sideHoldSpans(4), std::out_of_range);
    CHECK_THROWS_AS(frame.sliderHoldSpans(std::numeric_limits<std::size_t>::max()), std::out_of_range);
    CHECK_THROWS_AS(frame.sideHoldSpans(std::numeric_limits<std::size_t>::max()), std::out_of_range);
    s3d::Array<playfield::HoldSection> sections;
    sections.reserve(16);
    const auto capacity = sections.capacity();
    for (const auto spans : { frame.sliderHoldSpans(1), frame.sideHoldSpans(1) }) {
        REQUIRE(spans.size() == 5);
        frame.holdSections(spans.front(), sections);
        REQUIRE_FALSE(sections.isEmpty());
        frame.holdSections(spans[1], sections);
        CHECK(sections.isEmpty());
        sections.push_back({});
        frame.holdSections(spans[2], sections);
        CHECK(sections.isEmpty());
        frame.holdSections(spans[3], sections);
        CHECK_FALSE(sections.isEmpty());
        CHECK(sections.capacity() == capacity);
    }
}

TEST_CASE("Prepared holds own chart data and preserve large-sample interpolation", "[Playfield][HoldProjection]") {
    auto chart = ChartWithHolds();
    constexpr s3d::int64 Base = (s3d::int64{ 1 } << 53) + 33;
    for (auto& timeline : chart.timelines) {
        for (auto& change : timeline.speed_changes) {
            change.sample += Base;
        }
    }
    for (auto& hold : chart.slider_holds) {
        for (auto& point : hold.points) {
            point.sample += Base;
        }
    }
    for (auto& hold : chart.side_holds) {
        for (auto& point : hold.points) {
            point.sample += Base;
        }
    }
    const auto original = chart;
    const playfield::ChartProjection projection{ chart };
    chart.slider_holds.clear();
    chart.side_holds.clear();
    chart.timelines.clear();
    s3d::Array<long double> positions;
    s3d::Array<playfield::HoldSection> sections;
    for (const auto offset : { -500, 75, 100, 101, 200, 375, 600, 900, -100 }) {
        const s3d::int64 current = Base + offset;
        const auto frame = projection.frameAt(current, positions);
        CheckSpans(projection, frame, frame.sliderHoldSpans(0), original.slider_holds[0].points, current, sections);
        CheckSpans(projection, frame, frame.sideHoldSpans(0), original.side_holds[0].points, current, sections);
    }
}

TEST_CASE("Prepared hold sections validate timelines after skipping invalid pairs", "[Playfield][HoldProjection]") {
    const playfield::ChartProjection projection{ sheets::Chart{} };
    s3d::Array<long double> positions;
    const auto frame = projection.frameAt(50, positions);
    playfield::PreparedHoldSpan span{
        .before = { .sample = 0, .timeline = 1 },
        .after = { .sample = 100, .timeline = 1 },
        .first_sample = 0,
        .duration = 100.0,
    };
    s3d::Array<playfield::HoldSection> sections{ {} };
    sections.reserve(16);
    const auto capacity = sections.capacity();
    CHECK_THROWS_AS(frame.holdSections(span, sections), std::out_of_range);
    span.before.timeline = 0;
    CHECK_THROWS_AS(frame.holdSections(span, sections), std::out_of_range);
    span.before.sample = 100;
    CHECK_NOTHROW(frame.holdSections(span, sections));
    CHECK(sections.isEmpty());
    sections.push_back({});
    span.before.sample = 101;
    CHECK_NOTHROW(frame.holdSections(span, sections));
    CHECK(sections.isEmpty());
    CHECK(sections.capacity() == capacity);
}
