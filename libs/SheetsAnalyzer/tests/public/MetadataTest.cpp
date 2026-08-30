#include <ThirdParty/Catch2/catch.hpp>

#include <SheetsAnalyzer.hpp>

TEST_CASE("Metadata has stable defaults", "[SheetsAnalyzer][Metadata]") {
    const xlair::sheets::Metadata metadata;

    CHECK(metadata.title == U"Untitled");
    CHECK(metadata.title_sort == U"Untitled");
    CHECK(metadata.artist == U"Unknown Artist");
    CHECK(metadata.genre == U"Unspecified");
    CHECK(metadata.bpm == 120.0);
    CHECK(metadata.difficulties.isEmpty());
}

TEST_CASE("Chart load options use the engine sample rate", "[SheetsAnalyzer][Chart]") {
    const xlair::sheets::ChartLoadOptions options;

    CHECK(options.sample_rate == 44'100);
    CHECK(options.offset_seconds == 0.0);
}
