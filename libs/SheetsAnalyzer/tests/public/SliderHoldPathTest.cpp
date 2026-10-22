#include <ThirdParty/Catch2/catch.hpp>

#include <SheetsAnalyzer/SliderHoldPath.hpp>

namespace sheets = xlair::sheets;

TEST_CASE("Slider hold lane evaluation handles empty paths and endpoints", "[SheetsAnalyzer][HoldPath]") {
    CHECK_FALSE(sheets::EvaluateSliderHoldLane({}, 0).has_value());
    const s3d::Array<sheets::SliderHoldPoint> points{
        { .sample = 100, .lane = { 0, 2 } },
        { .sample = 300, .lane = { 12, 4 } },
    };
    for (const auto sample : { 0, 100 }) {
        const auto lane = sheets::EvaluateSliderHoldLane(points, sample);
        REQUIRE(lane.has_value());
        CHECK(lane->start == 0);
        CHECK(lane->width == 2);
    }
    for (const auto sample : { 300, 400 }) {
        const auto lane = sheets::EvaluateSliderHoldLane(points, sample);
        REQUIRE(lane.has_value());
        CHECK(lane->start == 12);
        CHECK(lane->width == 4);
    }
}

TEST_CASE("Slider hold lane evaluation interpolates both edges and rounds outward", "[SheetsAnalyzer][HoldPath]") {
    const s3d::Array<sheets::SliderHoldPoint> points{
        { .sample = 100, .lane = { 0, 2 } },
        { .sample = 300, .lane = { 12, 4 } },
    };
    const auto quarter = sheets::EvaluateSliderHoldLane(points, 150);
    REQUIRE(quarter.has_value());
    CHECK(quarter->start == 3);
    CHECK(quarter->width == 3);
    const auto middle = sheets::EvaluateSliderHoldLane(points, 200);
    REQUIRE(middle.has_value());
    CHECK(middle->start == 6);
    CHECK(middle->width == 3);
}

TEST_CASE("Slider hold lane evaluation chooses the last coincident point", "[SheetsAnalyzer][HoldPath]") {
    const s3d::Array<sheets::SliderHoldPoint> points{
        { .sample = 0, .lane = { 0, 2 } },
        { .sample = 100, .lane = { 4, 2 } },
        { .sample = 100, .lane = { 8, 2 } },
        { .sample = 200, .lane = { 12, 2 } },
    };
    const auto lane = sheets::EvaluateSliderHoldLane(points, 100);
    REQUIRE(lane.has_value());
    CHECK(lane->start == 8);
    CHECK(lane->width == 2);
}
