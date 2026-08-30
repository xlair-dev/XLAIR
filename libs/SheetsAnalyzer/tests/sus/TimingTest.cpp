#include <ThirdParty/Catch2/catch.hpp>

#include "formats/sus/Timing.hpp"

#include <limits>

namespace sus = xlair::sheets::formats::sus;

TEST_CASE("TimingMap converts default four-beat measures to samples", "[SheetsAnalyzer][SUS][Timing]") {
    const sus::Document document;
    const auto timing = sus::TimingMap::Build(document, {
                                                            .sample_rate = 100,
                                                            .offset_seconds = 0.25,
                                                        });

    REQUIRE(timing);
    CHECK(timing->toSample({ .measure = 0, .numerator = 0, .denominator = 1 }) == 25);
    CHECK(timing->toSample({ .measure = 0, .numerator = 1, .denominator = 2 }) == 125);
    CHECK(timing->toSample({ .measure = 1, .numerator = 0, .denominator = 1 }) == 225);
    CHECK(timing->bpmAt({ .measure = 10, .numerator = 0, .denominator = 1 }) == 120.0);
}

TEST_CASE("TimingMap applies measure lengths and chronologically sorted BPM changes", "[SheetsAnalyzer][SUS][Timing]") {
    sus::Document document;
    document.beats_per_measure[1] = 3.0;
    document.beats_per_measure[3] = 2.0;
    document.bpm_definitions[1] = 60.0;
    document.bpm_definitions[2] = 240.0;
    document.bpm_changes = {
        {
            .position = { .measure = 1, .numerator = 1, .denominator = 3 },
            .definition = 2,
        },
        {
            .position = { .measure = 0, .numerator = 1, .denominator = 2 },
            .definition = 1,
        },
    };

    const auto timing = sus::TimingMap::Build(document, { .sample_rate = 120 });

    REQUIRE(timing);
    CHECK(timing->toSample({ .measure = 0, .numerator = 1, .denominator = 2 }) == 120);
    CHECK(timing->toSample({ .measure = 1, .numerator = 0, .denominator = 1 }) == 360);
    CHECK(timing->toSample({ .measure = 1, .numerator = 1, .denominator = 3 }) == 480);
    CHECK(timing->toSample({ .measure = 2, .numerator = 0, .denominator = 1 }) == 540);
    CHECK(timing->toSample({ .measure = 3, .numerator = 0, .denominator = 1 }) == 630);
    CHECK(timing->toSample({ .measure = 4, .numerator = 0, .denominator = 1 }) == 690);

    CHECK(timing->bpmAt({ .measure = 0, .numerator = 0, .denominator = 1 }) == 120.0);
    CHECK(timing->bpmAt({ .measure = 0, .numerator = 1, .denominator = 2 }) == 60.0);
    CHECK(timing->bpmAt({ .measure = 1, .numerator = 1, .denominator = 3 }) == 240.0);
}

TEST_CASE("TimingMap handles distant measures without scanning every measure", "[SheetsAnalyzer][SUS][Timing]") {
    sus::Document document;
    document.beats_per_measure[1'000'000] = 3.0;

    const auto timing = sus::TimingMap::Build(document, { .sample_rate = 1 });

    REQUIRE(timing);
    CHECK(timing->toSample({ .measure = 1'000'000, .numerator = 0, .denominator = 1 }) == 2'000'000);
    CHECK(timing->toSample({ .measure = 1'000'001, .numerator = 0, .denominator = 1 }) == 2'000'002);
}

TEST_CASE("TimingMap clamps samples to the int64 range", "[SheetsAnalyzer][SUS][Timing]") {
    const auto maximum = sus::TimingMap::Build({}, {
                                                       .sample_rate = 1,
                                                       .offset_seconds = std::numeric_limits<double>::max(),
                                                   });
    const auto minimum = sus::TimingMap::Build({}, {
                                                       .sample_rate = 1,
                                                       .offset_seconds = std::numeric_limits<double>::lowest(),
                                                   });

    REQUIRE(maximum);
    REQUIRE(minimum);
    CHECK(maximum->toSample({}) == std::numeric_limits<s3d::int64>::max());
    CHECK(minimum->toSample({}) == std::numeric_limits<s3d::int64>::min());
}

TEST_CASE("TimingMap validates timing inputs", "[SheetsAnalyzer][SUS][Timing]") {
    SECTION("sample rate") {
        CHECK_FALSE(sus::TimingMap::Build({}, { .sample_rate = 0 }));
    }

    SECTION("offset") {
        CHECK_FALSE(sus::TimingMap::Build({}, { .offset_seconds = std::numeric_limits<double>::infinity() }));
    }

    SECTION("measure length") {
        sus::Document document;
        document.beats_per_measure[0] = 0.0;
        CHECK_FALSE(sus::TimingMap::Build(document, {}));
    }

    SECTION("undefined BPM") {
        sus::Document document;
        document.bpm_changes.push_back({
            .position = { .measure = 0, .numerator = 0, .denominator = 1 },
            .definition = 1,
        });
        CHECK_FALSE(sus::TimingMap::Build(document, {}));
    }

    SECTION("invalid BPM") {
        sus::Document document;
        document.bpm_definitions[1] = -120.0;
        document.bpm_changes.push_back({
            .position = { .measure = 0, .numerator = 0, .denominator = 1 },
            .definition = 1,
        });
        CHECK_FALSE(sus::TimingMap::Build(document, {}));
    }

    SECTION("invalid position") {
        sus::Document document;
        document.bpm_definitions[1] = 120.0;
        document.bpm_changes.push_back({
            .position = { .measure = 0, .numerator = 1, .denominator = 1 },
            .definition = 1,
        });
        CHECK_FALSE(sus::TimingMap::Build(document, {}));
    }
}
