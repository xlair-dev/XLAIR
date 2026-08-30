#include <Playfield/ScrollTimeline.hpp>
#include <ThirdParty/Catch2/catch.hpp>

#include <limits>
#include <stdexcept>

namespace {
    constexpr s3d::int64 SampleRate = 100;
}

TEST_CASE("ScrollTimeline uses normal speed when no changes are present", "[Playfield][ScrollTimeline]") {
    const xlair::playfield::ScrollTimeline timeline{ xlair::sheets::Timeline{}, SampleRate };

    CHECK(static_cast<double>(timeline.positionAt(250)) == Approx(2.5));
    CHECK(static_cast<double>(timeline.distanceBetween(50, 250)) == Approx(2.0));
}

TEST_CASE("ScrollTimeline integrates speed changes", "[Playfield][ScrollTimeline]") {
    const xlair::sheets::Timeline source{
        .speed_changes = {
            { .sample = 0, .multiplier = 1.0 },
            { .sample = 100, .multiplier = 2.0 },
            { .sample = 200, .multiplier = 0.5 },
        },
    };
    const xlair::playfield::ScrollTimeline timeline{ source, SampleRate };

    CHECK(static_cast<double>(timeline.positionAt(50)) == Approx(0.5));
    CHECK(static_cast<double>(timeline.positionAt(150)) == Approx(2.0));
    CHECK(static_cast<double>(timeline.positionAt(300)) == Approx(3.5));
    CHECK(static_cast<double>(timeline.distanceBetween(300, 50)) == Approx(-3.0));
}

TEST_CASE("ScrollTimeline supports stops and reverse scrolling", "[Playfield][ScrollTimeline]") {
    const xlair::sheets::Timeline source{
        .speed_changes = {
            { .sample = 0, .multiplier = 1.0 },
            { .sample = 100, .multiplier = 0.0 },
            { .sample = 200, .multiplier = -1.0 },
        },
    };
    const xlair::playfield::ScrollTimeline timeline{ source, SampleRate };

    CHECK(static_cast<double>(timeline.positionAt(100)) == Approx(1.0));
    CHECK(static_cast<double>(timeline.positionAt(200)) == Approx(1.0));
    CHECK(static_cast<double>(timeline.positionAt(300)) == Approx(0.0));
}

TEST_CASE("ScrollTimeline sorts changes and uses the last change at the same sample", "[Playfield][ScrollTimeline]") {
    const xlair::sheets::Timeline source{
        .speed_changes = {
            { .sample = 100, .multiplier = 2.0 },
            { .sample = 0, .multiplier = 1.0 },
            { .sample = 100, .multiplier = 3.0 },
        },
    };
    const xlair::playfield::ScrollTimeline timeline{ source, SampleRate };

    CHECK(static_cast<double>(timeline.positionAt(50)) == Approx(0.5));
    CHECK(static_cast<double>(timeline.positionAt(200)) == Approx(4.0));
}

TEST_CASE("ScrollTimeline uses normal speed before its first change", "[Playfield][ScrollTimeline]") {
    const xlair::sheets::Timeline source{
        .speed_changes = {
            { .sample = 100, .multiplier = 2.0 },
        },
    };
    const xlair::playfield::ScrollTimeline timeline{ source, SampleRate };

    CHECK(static_cast<double>(timeline.positionAt(0)) == Approx(-1.0));
    CHECK(static_cast<double>(timeline.distanceBetween(0, 150)) == Approx(2.0));
}

TEST_CASE("ScrollTimeline rejects invalid inputs", "[Playfield][ScrollTimeline]") {
    CHECK_THROWS_AS(xlair::playfield::ScrollTimeline(xlair::sheets::Timeline{}, 0), std::invalid_argument);

    const xlair::sheets::Timeline source{
        .speed_changes = {
            { .sample = 0, .multiplier = std::numeric_limits<double>::infinity() },
        },
    };
    CHECK_THROWS_AS(xlair::playfield::ScrollTimeline(source, SampleRate), std::invalid_argument);
}
