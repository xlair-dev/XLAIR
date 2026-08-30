#include <ThirdParty/Catch2/catch.hpp>

#include <SheetsAnalyzer.hpp>

TEST_CASE("LoadChart rejects unsupported formats", "[SheetsAnalyzer][Chart]") {
    const auto result = xlair::sheets::LoadChart(U"chart.txt");

    REQUIRE_FALSE(result);
    REQUIRE(result.diagnostics.size() == 1);
    CHECK(result.diagnostics.front().severity == xlair::sheets::DiagnosticSeverity::Error);
    CHECK(result.diagnostics.front().path == U"chart.txt");
}
