#include <Playfield/ChartProjection.hpp>
#include <ThirdParty/Catch2/catch.hpp>

#include <stdexcept>

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

TEST_CASE("ChartProjection rejects an invalid timeline index", "[Playfield][ChartProjection]") {
    const xlair::playfield::ChartProjection projection{ xlair::sheets::Chart{} };

    CHECK_THROWS_AS(projection.positionAt(1, 0), std::out_of_range);
    CHECK_THROWS_AS(projection.noteDistanceBreakpoints(1, 0, 0, 100), std::out_of_range);
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
