#include <Playfield/ChartProjection.hpp>
#include <ThirdParty/Catch2/catch.hpp>

#include <array>
#include <stdexcept>

TEST_CASE("ChartProjection frames exactly match uncached distances", "[Playfield][ChartProjection]") {
    const xlair::sheets::Chart chart{
        .sample_rate = 44'100,
        .timelines = {
            {},
            { .speed_changes = {
                { .sample = -100, .multiplier = 1.25 },
                { .sample = 0, .multiplier = 0.0 },
                { .sample = 100, .multiplier = -0.75 },
                { .sample = 200, .multiplier = 2.0 },
                { .sample = 200, .multiplier = 0.1 },
                { .sample = 300, .multiplier = 1.0 },
            } },
            { .speed_changes = {
                { .sample = 100, .multiplier = 1.0 / 3.0 },
                { .sample = 200, .multiplier = -2.5 },
            } },
        },
    };
    const xlair::playfield::ChartProjection projection{ chart };
    constexpr std::array<s3d::int64, 12> current_samples{ 0, 200, -150, 1000, 100, 99, 300, -100, 201, 200, -1, 301 };
    constexpr std::array<s3d::int64, 16> note_samples{
        -250, -101, -100, -99, -1, 0, 99, 100, 101, 199, 200, 201, 299, 300, 301, 1000,
    };
    s3d::Array<long double> current_positions;
    current_positions.reserve(16);
    const auto capacity = current_positions.capacity();

    for (const auto current : current_samples) {
        const auto frame = projection.frameAt(current, current_positions);
        REQUIRE(current_positions.size() == projection.timelineCount());
        CHECK(current_positions.capacity() == capacity);
        for (std::size_t timeline = 0; timeline < projection.timelineCount(); ++timeline) {
            CAPTURE(current, timeline);
            CHECK(current_positions[timeline] == projection.positionAt(timeline, current));
            CHECK(frame.noteDistance(timeline, current) == projection.noteDistance(timeline, current, current));
            for (const auto note : note_samples) {
                CAPTURE(note);
                CHECK(frame.noteDistance(timeline, note) == projection.noteDistance(timeline, current, note));
            }
        }
    }
}

TEST_CASE("ChartProjection frames share breakpoint and invalid-index semantics", "[Playfield][ChartProjection]") {
    const xlair::sheets::Chart chart{
        .sample_rate = 100,
        .timelines = {
            { .speed_changes = {
                { .sample = 0, .multiplier = 0.0 },
                { .sample = 100, .multiplier = -1.0 },
                { .sample = 200, .multiplier = 2.0 },
            } },
        },
    };
    const xlair::playfield::ChartProjection projection{ chart };
    s3d::Array<long double> current_positions;
    const auto frame = projection.frameAt(100, current_positions);
    s3d::Array<s3d::int64> samples{ 999 };
    samples.reserve(16);
    const auto capacity = samples.capacity();
    const auto check_range = [&](const s3d::int64 from, const s3d::int64 to) {
        frame.noteDistanceBreakpoints(0, from, to, samples);
        CHECK(samples == projection.noteDistanceBreakpoints(0, 100, from, to));
        CHECK(samples.capacity() == capacity);
    };

    check_range(-100, 300);
    check_range(0, 200);
    check_range(100, 200);
    check_range(200, 100);
    check_range(100, 100);
    CHECK_THROWS_AS(frame.noteDistance(1, 99), std::out_of_range);
    CHECK_THROWS_AS(frame.noteDistance(1, 100), std::out_of_range);
    CHECK_THROWS_AS(frame.noteDistance(1, 101), std::out_of_range);
    samples.push_back(999);
    CHECK_THROWS_AS(frame.noteDistanceBreakpoints(1, 0, 300, samples), std::out_of_range);
    CHECK(samples.isEmpty());
    samples.push_back(999);
    CHECK_THROWS_AS(frame.noteDistanceBreakpoints(1, 100, 100, samples), std::out_of_range);
    CHECK(samples.isEmpty());
    CHECK(samples.capacity() == capacity);
}

TEST_CASE("ChartProjection frames reuse storage after switching projections", "[Playfield][ChartProjection]") {
    const xlair::playfield::ChartProjection empty_projection{ xlair::sheets::Chart{ .sample_rate = 44'100 } };
    const xlair::playfield::ChartProjection other_projection{
        xlair::sheets::Chart{
            .sample_rate = 48'000,
            .timelines = {
                { .speed_changes = { { .sample = 0, .multiplier = 0.0 } } },
                { .speed_changes = { { .sample = -100, .multiplier = -2.0 } } },
            },
        },
    };
    s3d::Array<long double> current_positions{ 999.0L, 999.0L, 999.0L };
    current_positions.reserve(16);
    const auto capacity = current_positions.capacity();
    const auto check_projection = [&](const xlair::playfield::ChartProjection& projection, const s3d::int64 current) {
        const auto frame = projection.frameAt(current, current_positions);
        CHECK(current_positions.size() == projection.timelineCount());
        CHECK(current_positions.capacity() == capacity);
        for (std::size_t timeline = 0; timeline < projection.timelineCount(); ++timeline) {
            CHECK(frame.noteDistance(timeline, -1000) == projection.noteDistance(timeline, current, -1000));
            CHECK(frame.noteDistance(timeline, current) == projection.noteDistance(timeline, current, current));
            CHECK(frame.noteDistance(timeline, 1000) == projection.noteDistance(timeline, current, 1000));
        }
    };

    check_projection(empty_projection, -200);
    check_projection(other_projection, 200);
    check_projection(empty_projection, 100);
    check_projection(other_projection, -100);
}

TEST_CASE("ChartProjection keeps independent scroll timelines", "[Playfield][ChartProjection]") {
    const xlair::sheets::Chart chart{
        .sample_rate = 100,
        .timelines = {
            { .speed_changes = { { .sample = 0, .multiplier = 1.0 } } },
            { .speed_changes = { { .sample = 0, .multiplier = 2.0 } } },
        },
    };
    const xlair::playfield::ChartProjection projection{ chart };

    CHECK(projection.timelineCount() == 2);
    CHECK(static_cast<double>(projection.positionAt(0, 100)) == Approx(1.0));
    CHECK(static_cast<double>(projection.positionAt(1, 100)) == Approx(2.0));
    CHECK(static_cast<double>(projection.noteDistance(0, 0, 100)) == Approx(1.0));
    CHECK(static_cast<double>(projection.noteDistance(1, 0, 100)) == Approx(2.0));
    CHECK(static_cast<double>(projection.noteDistance(0, 200, 100)) == Approx(-1.0));
    CHECK(static_cast<double>(projection.noteDistance(1, 200, 100)) == Approx(-1.0));
    CHECK(projection.noteDistanceBreakpoints(1, 50, 0, 100) == (s3d::Array<s3d::int64>{ 50 }));
}

TEST_CASE("ChartProjection supplies a normal timeline for an empty chart", "[Playfield][ChartProjection]") {
    const xlair::sheets::Chart chart{ .sample_rate = 100 };
    const xlair::playfield::ChartProjection projection{ chart };

    CHECK(projection.timelineCount() == 1);
    CHECK(static_cast<double>(projection.positionAt(0, 250)) == Approx(2.5));
}

TEST_CASE("ChartProjection reuses breakpoint output across timelines", "[Playfield][ChartProjection]") {
    const xlair::sheets::Chart chart{
        .sample_rate = 100,
        .timelines = {
            { .speed_changes = { { .sample = 0, .multiplier = 1.0 } } },
            { .speed_changes = {
                { .sample = 200, .multiplier = -1.0 },
                { .sample = 100, .multiplier = 1.0 },
                { .sample = 100, .multiplier = 0.0 },
            } },
        },
    };
    const xlair::playfield::ChartProjection projection{ chart };
    s3d::Array<s3d::int64> samples{ 999 };
    samples.reserve(16);
    const auto capacity = samples.capacity();

    projection.noteDistanceBreakpoints(1, 100, 0, 300, samples);
    CHECK(samples == (s3d::Array<s3d::int64>{ 100, 200 }));
    CHECK(samples == projection.noteDistanceBreakpoints(1, 100, 0, 300));
    projection.noteDistanceBreakpoints(0, 50, 0, 100, samples);
    CHECK(samples == s3d::Array<s3d::int64>{ 50 });
    projection.noteDistanceBreakpoints(1, 250, 0, 200, samples);
    CHECK(samples.isEmpty());
    projection.noteDistanceBreakpoints(1, 0, 200, 200, samples);
    CHECK(samples.isEmpty());
    CHECK(samples.capacity() == capacity);
}

TEST_CASE("ChartProjection rejects an invalid timeline index", "[Playfield][ChartProjection]") {
    const xlair::playfield::ChartProjection projection{ xlair::sheets::Chart{} };

    CHECK_THROWS_AS(projection.positionAt(1, 0), std::out_of_range);
    CHECK_THROWS_AS(projection.noteDistanceBreakpoints(1, 0, 0, 100), std::out_of_range);

    s3d::Array<s3d::int64> samples{ 999 };
    samples.reserve(16);
    const auto capacity = samples.capacity();
    CHECK_THROWS_AS(projection.noteDistanceBreakpoints(1, 0, 0, 100, samples), std::out_of_range);
    CHECK(samples.isEmpty());
    CHECK(samples.capacity() == capacity);
    samples.push_back(999);
    CHECK_THROWS_AS(projection.noteDistanceBreakpoints(1, 0, 100, 100, samples), std::out_of_range);
    CHECK_THROWS_AS(projection.noteDistanceBreakpoints(1, 0, 100, 100), std::out_of_range);
    CHECK(samples.isEmpty());
    projection.noteDistanceBreakpoints(0, 50, 0, 100, samples);
    CHECK(samples == s3d::Array<s3d::int64>{ 50 });
}

TEST_CASE("Measure lines use the main timeline through stops and reverse scroll", "[Playfield][ChartProjection]") {
    const xlair::sheets::Chart chart{
        .sample_rate = 1000,
        .timelines = {
            { .speed_changes = { { .sample = 0, .multiplier = 2.0 } } },
            { .speed_changes = {
                { .sample = 0, .multiplier = 1.0 },
                { .sample = 1000, .multiplier = 0.0 },
                { .sample = 2000, .multiplier = -1.0 },
                { .sample = 3000, .multiplier = 1.0 },
            } },
        },
        .main_timeline = 1,
        .measure_lines = { { .measure = 1, .sample = 1500 }, { .measure = 2, .sample = 2500 } },
    };
    const xlair::playfield::ChartProjection projection{ chart };
    const auto distance = [&](const s3d::int64 now, const std::size_t index) {
        return static_cast<double>(
            projection.noteDistance(chart.main_timeline, now, chart.measure_lines[index].sample)
        );
    };
    CHECK(distance(500, 0) == Approx(0.5));
    CHECK(distance(1000, 1) == Approx(-0.5));
    CHECK(distance(1500, 1) == Approx(-0.5));
    CHECK(distance(2500, 1) == Approx(0.0));
    // Passed lines use the same fixed 1.0x tail as passed notes.
    CHECK(distance(3000, 1) == Approx(-0.5));
    CHECK(static_cast<double>(projection.noteDistance(0, 500, chart.measure_lines[0].sample)) == Approx(2.0));
}
