#include <ThirdParty/Catch2/catch.hpp>

#include "Converter.hpp"
#include "compilation/Compiler.hpp"

namespace {
    [[nodiscard]]
    xlair::sheets::Result<xlair::sheets::Chart>
    CompileSource(const std::string_view source, const xlair::sheets::ChartLoadOptions& options = {}) {
        const auto parsed = fixtures::ParseChart(source);
        if (const auto error = cc_result_error(parsed.get())) {
            return xlair::sheets::Result<xlair::sheets::Chart>::makeError(s3d::Unicode::FromUTF8(error));
        }
        return xlair::sheets::compilation::Compile(cc_result_chart(parsed.get()), options);
    }
}

TEST_CASE("Compile includes off-grid side hold relays in judgement samples", "[SheetsAnalyzer][Compilation]") {
    const auto result = CompileSource("#00020A: 143100\n#00120A: 24\n", { .sample_rate = 100 });
    REQUIRE(result);
    REQUIRE(result->side_holds.size() == 1);
    const s3d::Array<s3d::int64> expected = { 0, 25, 50, 67, 75, 100, 125, 150, 175, 200 };
    CHECK(result->side_holds.front().judge_samples == expected);
    CHECK(result->total_combo == expected.size());
}

TEST_CASE("Compile shares a side X-tap parent without retaining the slider note", "[SheetsAnalyzer][Compilation]") {
    const auto result = CompileSource(
        "#TIL00: \"0'0:0.5\"\n#TIL01: \"0'0:2.0\"\n"
        "#HISPEED 00\n#00110: 24\n#HISPEED 01\n#00150: 34\n#00150: 54\n"
    );
    REQUIRE(result);
    CHECK(result->slider_notes.isEmpty());
    REQUIRE(result->side_notes.size() == 2);
    CHECK(result->side_notes[0].button == xlair::sheets::SideButton::LeftUpper);
    CHECK(result->side_notes[1].button == xlair::sheets::SideButton::LeftLower);
    for (const auto& note : result->side_notes) {
        CHECK(note.kind == xlair::sheets::SideNoteKind::XTap);
        REQUIRE(note.timeline < result->timelines.size());
        CHECK(result->timelines[note.timeline].speed_changes.back().multiplier == 0.5);
    }
    CHECK(result->total_combo == 2);
}

TEST_CASE("Compile applies converter TIL positions and the game offset", "[SheetsAnalyzer][Compilation]") {
    const auto result = CompileSource(
        "#REQUEST \"ticks_per_beat 480\"\n#TIL00: \"0'240:2.0\"\n#HISPEED 00\n#00110: 14\n",
        { .sample_rate = 480, .offset_seconds = 0.5 }
    );
    REQUIRE(result);
    REQUIRE(result->slider_notes.size() == 1);
    const auto& note = result->slider_notes.front();
    CHECK(note.sample == 1200);
    const auto& speeds = result->timelines.at(note.timeline).speed_changes;
    REQUIRE(speeds.size() == 2);
    CHECK(speeds.front().sample == 240);
    CHECK(speeds.back().sample == 720);
    CHECK(speeds.back().multiplier == 2.0);
}

TEST_CASE("Compile rejects hold anchors collapsed by sample rounding", "[SheetsAnalyzer][Compilation]") {
    SECTION("slider") {
        CHECK_FALSE(CompileSource("#BPM01: 600000\n#00008: 01\n#00030A: 14\n#00130A: 24\n", { .sample_rate = 1 }));
    }
    SECTION("side") {
        CHECK_FALSE(CompileSource("#BPM01: 600000\n#00008: 01\n#00020A: 14\n#00120A: 24\n", { .sample_rate = 1 }));
    }
}

TEST_CASE("Compile rejects converter note kinds the game cannot represent", "[SheetsAnalyzer][Compilation]") {
    const auto result = CompileSource("#00050: 74\n");
    REQUIRE_FALSE(result);
    REQUIRE(result.diagnostics.size() == 1);
    CHECK(result.diagnostics.front().severity == xlair::sheets::DiagnosticSeverity::Error);
}
