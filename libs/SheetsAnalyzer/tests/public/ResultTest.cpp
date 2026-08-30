#include <ThirdParty/Catch2/catch.hpp>

#include <SheetsAnalyzer/Result.hpp>

TEST_CASE("Result reports whether it contains a value", "[SheetsAnalyzer][Result]") {
    xlair::sheets::Result<s3d::int32> result;

    CHECK_FALSE(result.hasValue());
    CHECK_FALSE(result);

    result.value = 42;

    REQUIRE(result.hasValue());
    REQUIRE(result);
    CHECK(*result == 42);
}

TEST_CASE("Result constructs a successful result from a value", "[SheetsAnalyzer][Result]") {
    const xlair::sheets::Result<s3d::int32> result{ 42 };

    REQUIRE(result);
    CHECK(*result == 42);
    CHECK(result.diagnostics.isEmpty());
}

TEST_CASE("Result creates an error with source information", "[SheetsAnalyzer][Result]") {
    const auto result = xlair::sheets::Result<s3d::int32>::makeError(U"Invalid value.", U"chart.sus", 12, 7);

    REQUIRE_FALSE(result);
    REQUIRE(result.diagnostics.size() == 1);
    CHECK(result.diagnostics.front().severity == xlair::sheets::DiagnosticSeverity::Error);
    CHECK(result.diagnostics.front().message == U"Invalid value.");
    CHECK(result.diagnostics.front().path == U"chart.sus");
    CHECK(*result.diagnostics.front().line == 12);
    CHECK(*result.diagnostics.front().column == 7);
}
