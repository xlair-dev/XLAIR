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
