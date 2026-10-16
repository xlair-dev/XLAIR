#include <ThirdParty/Catch2/catch.hpp>

#include "Converter.hpp"
#include "compilation/Timing.hpp"

#include <limits>

namespace compilation = xlair::sheets::compilation;

TEST_CASE("TimingMap converts absolute beats with the game offset", "[SheetsAnalyzer][Compilation][Timing]") {
    const auto parsed = fixtures::ParseChart("");
    REQUIRE(cc_result_error(parsed.get()) == nullptr);
    const auto timing =
        compilation::TimingMap::Build(cc_result_chart(parsed.get()), { .sample_rate = 100, .offset_seconds = 0.25 });
    REQUIRE(timing);
    CHECK(timing->toSample({ 0, 1 }) == 25);
    CHECK(timing->toSample({ 2, 1 }) == 125);
    CHECK(timing->toSample({ 4, 1 }) == 225);
    REQUIRE(timing->tempoChanges().size() == 1);
    CHECK(timing->tempoChanges().front().bpm == 120.0);
}

TEST_CASE("TimingMap integrates converter tempos in musical order", "[SheetsAnalyzer][Compilation][Timing]") {
    const auto parsed =
        fixtures::ParseChart("#BPM01: 60\n#BPM02: 240\n#00102: 3\n#00302: 2\n#00108: 000200\n#00008: 0001\n");
    REQUIRE(cc_result_error(parsed.get()) == nullptr);
    const auto timing = compilation::TimingMap::Build(cc_result_chart(parsed.get()), { .sample_rate = 120 });
    REQUIRE(timing);
    CHECK(timing->toSample({ 2, 1 }) == 120);
    CHECK(timing->toSample({ 4, 1 }) == 360);
    CHECK(timing->toSample({ 5, 1 }) == 480);
    CHECK(timing->toSample({ 7, 1 }) == 540);
    CHECK(timing->toSample({ 10, 1 }) == 630);
    CHECK(timing->toSample({ 12, 1 }) == 690);
    const auto tempos = timing->tempoChanges();
    REQUIRE(tempos.size() == 3);
    CHECK(tempos[1].bpm == 60.0);
    CHECK(tempos[2].bpm == 240.0);
}

TEST_CASE("TimingMap clamps samples to the int64 range", "[SheetsAnalyzer][Compilation][Timing]") {
    const auto parsed = fixtures::ParseChart("");
    REQUIRE(cc_result_error(parsed.get()) == nullptr);
    const auto maximum = compilation::TimingMap::Build(
        cc_result_chart(parsed.get()),
        {
            .sample_rate = 1,
            .offset_seconds = std::numeric_limits<double>::max(),
        }
    );
    const auto minimum = compilation::TimingMap::Build(
        cc_result_chart(parsed.get()),
        {
            .sample_rate = 1,
            .offset_seconds = std::numeric_limits<double>::lowest(),
        }
    );
    REQUIRE(maximum);
    REQUIRE(minimum);
    CHECK(maximum->toSample({ 0, 1 }) == std::numeric_limits<s3d::int64>::max());
    CHECK(minimum->toSample({ 0, 1 }) == std::numeric_limits<s3d::int64>::min());
}

TEST_CASE("TimingMap validates game timing options", "[SheetsAnalyzer][Compilation][Timing]") {
    const auto parsed = fixtures::ParseChart("");
    REQUIRE(cc_result_error(parsed.get()) == nullptr);
    const auto source = cc_result_chart(parsed.get());
    CHECK_FALSE(compilation::TimingMap::Build(source, { .sample_rate = 0 }));
    CHECK_FALSE(compilation::TimingMap::Build(source, { .sample_rate = -1 }));
    CHECK_FALSE(compilation::TimingMap::Build(source, { .offset_seconds = std::numeric_limits<double>::infinity() }));
    CHECK_FALSE(compilation::TimingMap::Build(source, { .offset_seconds = std::numeric_limits<double>::quiet_NaN() }));
}
