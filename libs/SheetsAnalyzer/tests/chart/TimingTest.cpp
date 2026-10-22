#include <ThirdParty/Catch2/catch.hpp>

#include "chart/SampleMath.hpp"
#include "chart/Timing.hpp"

#include <cmath>
#include <limits>

namespace compiler = xlair::sheets::chart;

TEST_CASE("IR timing clamps samples without integer conversion overflow", "[SheetsAnalyzer][IR][Timing]") {
    const auto maximum =
        compiler::TimingMap::Build({}, { .sample_rate = 1, .offset_seconds = std::numeric_limits<double>::max() });
    const auto minimum =
        compiler::TimingMap::Build({}, { .sample_rate = 1, .offset_seconds = std::numeric_limits<double>::lowest() });
    REQUIRE(maximum);
    REQUIRE(minimum);
    CHECK(maximum->toSample({ 0, 1 }) == std::numeric_limits<s3d::int64>::max());
    CHECK(minimum->toSample({ 0, 1 }) == std::numeric_limits<s3d::int64>::min());
    const long double limit = std::ldexp(1.0L, 63);
    CHECK(compiler::detail::RoundSample(limit) == std::numeric_limits<s3d::int64>::max());
    CHECK(compiler::detail::RoundSample(-limit) == std::numeric_limits<s3d::int64>::min());
    CHECK(compiler::detail::RoundSample(-0.5L) == -1);
    CHECK(compiler::detail::RoundSample(0.5L) == 1);
}

TEST_CASE("IR timing rejects invalid or unsorted tempo records", "[SheetsAnalyzer][IR][Timing]") {
    for (const double bpm : { 0.0, -120.0, std::numeric_limits<double>::infinity() }) {
        const CcTempo tempos[]{ { { 0, 1 }, bpm } };
        CHECK_FALSE(compiler::TimingMap::Build(tempos, {}));
    }
    const CcTempo zero_denominator[]{ { { 1, 0 }, 120.0 } };
    CHECK_FALSE(compiler::TimingMap::Build(zero_denominator, {}));
    const CcTempo reversed[]{ { { 4, 1 }, 120.0 }, { { 2, 1 }, 120.0 } };
    CHECK_FALSE(compiler::TimingMap::Build(reversed, {}));
}
