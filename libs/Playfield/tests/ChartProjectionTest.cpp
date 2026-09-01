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
}
