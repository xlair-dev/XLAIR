#include <ThirdParty/Catch2/catch.hpp>

#include "chart/Loader.hpp"

#include <string_view>

namespace sheets = xlair::sheets;

namespace {
    sheets::Result<sheets::Chart> ParseMeasures(
        const std::string_view text,
        const CcFormat format = CcFormat_Sus,
        const sheets::ChartLoadOptions options = { .sample_rate = 480 }
    ) {
        return sheets::chart::Parse(
            { reinterpret_cast<const s3d::uint8*>(text.data()), text.size() },
            format,
            options,
            U"measures"
        );
    }
}

TEST_CASE("Measure lines include empty measures and the boundary after the final note", "[SheetsAnalyzer][Measures]") {
    const auto chart = ParseMeasures(R"(#BPM01:120
#00008:01
#00210:14
)");
    REQUIRE(chart);
    REQUIRE(chart->measure_lines.size() == 4);
    for (s3d::uint32 index = 0; index < 4; ++index) {
        CHECK(chart->measure_lines[index].measure == index);
        CHECK(chart->measure_lines[index].sample == index * 960);
    }
    CHECK(chart->total_combo == 1);
    CHECK(chart->main_timeline == 0);
}

TEST_CASE("Measure sample timing integrates BPM changes within a measure", "[SheetsAnalyzer][Measures]") {
    const auto chart = ParseMeasures(R"(#BPM01:120
#BPM02:240
#00008:0102
#00110:14
)");
    REQUIRE(chart);
    REQUIRE(chart->measure_lines.size() == 3);
    CHECK(chart->measure_lines[0].sample == 0);
    CHECK(chart->measure_lines[1].sample == 720);
    CHECK(chart->measure_lines[2].sample == 1200);
    CHECK(chart->slider_notes.front().sample == chart->measure_lines[1].sample);
}

TEST_CASE("SUS and UGC measure lengths persist until the next change", "[SheetsAnalyzer][Measures]") {
    const auto sus = ParseMeasures(R"(#00002:3
#00102:5
#00310:14
)");
    const auto ugc = ParseMeasures(
        R"(@BEAT	0	3	4
@BEAT	1	5	4
@ENDHEAD
#3'0:t04
)",
        CcFormat_Ugc
    );
    REQUIRE(sus);
    REQUIRE(ugc);
    REQUIRE(sus->measure_lines.size() == 5);
    REQUIRE(ugc->measure_lines.size() == 5);
    constexpr s3d::int64 expected[]{ 0, 720, 1920, 3120, 4320 };
    for (s3d::uint32 index = 0; index < 5; ++index) {
        CHECK(sus->measure_lines[index].sample == expected[index]);
        CHECK(ugc->measure_lines[index].sample == expected[index]);
        CHECK(ugc->measure_lines[index].measure == index);
    }
}

TEST_CASE("Fractional measure lengths do not become a four-beat grid", "[SheetsAnalyzer][Measures]") {
    const auto chart = ParseMeasures(R"(#00002:3.5
#00310:14
)");
    REQUIRE(chart);
    REQUIRE(chart->measure_lines.size() == 5);
    for (s3d::uint32 index = 0; index < 5; ++index) {
        CHECK(chart->measure_lines[index].sample == index * 840);
    }
}

TEST_CASE("Measure lines use the same source and music offsets as notes", "[SheetsAnalyzer][Measures]") {
    const auto chart = ParseMeasures(
        R"(#WAVEOFFSET 0.3
#00110:14
)",
        CcFormat_Sus,
        { .sample_rate = 480, .offset_seconds = 0.2 }
    );
    REQUIRE(chart);
    REQUIRE(chart->measure_lines.size() == 3);
    CHECK(chart->measure_lines[0].sample == -48);
    CHECK(chart->measure_lines[1].sample == 912);
    CHECK(chart->measure_lines[2].sample == 1872);
    CHECK(chart->slider_notes.front().sample == chart->measure_lines[1].sample);
}

TEST_CASE(
    "Measure lines extend through hold tails rather than stopping at their starts",
    "[SheetsAnalyzer][Measures]"
) {
    const auto chart = ParseMeasures(R"(#00030A:14
#00334A:24
)");
    REQUIRE(chart);
    REQUIRE(chart->slider_holds.size() == 1);
    REQUIRE(chart->measure_lines.size() == 5);
    CHECK(chart->slider_holds.front().points.back().sample == 2880);
    CHECK(chart->measure_lines.back().sample == 3840);
}

TEST_CASE("An empty chart still contains its initial measure boundaries", "[SheetsAnalyzer][Measures]") {
    const auto chart = ParseMeasures(R"(#TITLE "Empty"
)");
    REQUIRE(chart);
    REQUIRE(chart->measure_lines.size() == 2);
    CHECK(chart->measure_lines[0].sample == 0);
    CHECK(chart->measure_lines[1].sample == 960);
    CHECK(chart->total_combo == 0);
}

TEST_CASE(
    "Missing main-timeline metadata uses the default independently of note HISPEED",
    "[SheetsAnalyzer][Measures]"
) {
    const auto chart = ParseMeasures(R"(#TIL01:"0'0:0.5"
#TIL02:"0'0:2"
#HISPEED 02
#00110:14
)");
    REQUIRE(chart);
    REQUIRE(chart->slider_notes.size() == 1);
    REQUIRE(chart->main_timeline < chart->timelines.size());
    CHECK(chart->main_timeline != chart->slider_notes[0].timeline);
    CHECK(chart->timelines[chart->main_timeline].speed_changes.back().multiplier == 1.0);
    CHECK(chart->timelines[chart->slider_notes[0].timeline].speed_changes.back().multiplier == 2.0);
}

TEST_CASE("UGC measure lines follow MAINTIL even when no note uses the group", "[SheetsAnalyzer][Measures]") {
    const auto chart = ParseMeasures(
        R"(@TIL	7	0'0	0.5
@TIL	2	0'0	2
@MAINTIL	7
@USETIL	2
@ENDHEAD
#1'0:t04
)",
        CcFormat_Ugc
    );
    REQUIRE(chart);
    REQUIRE(chart->slider_notes.size() == 1);
    REQUIRE(chart->main_timeline < chart->timelines.size());
    CHECK(chart->main_timeline != chart->slider_notes[0].timeline);
    CHECK(chart->timelines[chart->main_timeline].speed_changes.back().multiplier == 0.5);
}

TEST_CASE("SUS TIL00 note groups do not replace the default measure timeline", "[SheetsAnalyzer][Measures]") {
    const auto chart = ParseMeasures(R"(#TIL00:"0'0:0.5"
#HISPEED 00
#00110:14
)");
    REQUIRE(chart);
    REQUIRE(chart->slider_notes.size() == 1);
    CHECK(chart->main_timeline != chart->slider_notes[0].timeline);
    CHECK(chart->timelines[chart->main_timeline].speed_changes.back().multiplier == 1.0);
}
