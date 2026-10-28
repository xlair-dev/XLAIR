#include <ThirdParty/Catch2/catch.hpp>

#include "chart/Loader.hpp"
#include "chart/Timing.hpp"
#include "chart/SliderHoldCompiler.hpp"

#include <cmath>
#include <limits>
#include <string>
#include <string_view>
#include <future>
#include <vector>

namespace sheets = xlair::sheets;
namespace compiler = xlair::sheets::chart;

namespace {
    sheets::Result<sheets::Chart> Parse(
        const std::string_view text,
        const CcFormat format = CcFormat_Sus,
        const sheets::ChartLoadOptions options = { .sample_rate = 480 }
    ) {
        return compiler::Parse(
            { reinterpret_cast<const s3d::uint8*>(text.data()), text.size() },
            format,
            options,
            U"chart.sus"
        );
    }
}

TEST_CASE("IR loader consumes only exact Tap 1 carriers by default", "[SheetsAnalyzer][IR]") {
    for (const auto records : { "#00010:14\n#00050:34\n", "#00050:34\n#00010:14\n" }) {
        const auto result = Parse(records);
        REQUIRE(result);
        CHECK(result->slider_notes.isEmpty());
        REQUIRE(result->side_notes.size() == 1);
        CHECK(result->side_notes[0].kind == sheets::SideNoteKind::Tap);
        CHECK(result->total_combo == 1);
        const auto independent = Parse(std::string{ "#REQUEST \"xlair_directional_mode independent\"\n" } + records);
        REQUIRE(independent);
        CHECK(independent->slider_notes.size() == 1);
        CHECK(independent->total_combo == 2);
    }
    const auto mismatch = Parse("#00010:14\n#00050:33\n");
    REQUIRE(mismatch);
    CHECK(mismatch->total_combo == 2);
}

TEST_CASE("Side XTap IR pairs are a single side note without a main slider note", "[SheetsAnalyzer][IR]") {
    for (const auto records : { "#00010:24\n#00050:34\n", "#00050:34\n#00010:24\n" }) {
        const auto result = Parse(records);
        REQUIRE(result);
        CHECK(result->slider_notes.isEmpty());
        REQUIRE(result->side_notes.size() == 1);
        CHECK(result->side_notes[0].kind == sheets::SideNoteKind::XTap);
        CHECK(result->side_notes[0].button == sheets::SideButton::LeftUpper);
        CHECK(result->total_combo == 1);
    }
    const auto ugc = Parse("@ENDHEAD\n#0'0:x04\n#0'0:a04DRN\n", CcFormat_Ugc);
    REQUIRE(ugc);
    CHECK(ugc->slider_notes.isEmpty());
    REQUIRE(ugc->side_notes.size() == 1);
    CHECK(ugc->side_notes[0].button == sheets::SideButton::RightLower);
    CHECK(ugc->side_notes[0].kind == sheets::SideNoteKind::XTap);
    CHECK(ugc->total_combo == 1);
}

TEST_CASE("SUS and UGC compile through the same beat and sample pipeline", "[SheetsAnalyzer][IR]") {
    const auto sus = Parse(R"(#BPM01:120
#00008:01
#TIL00:"0'0:0.5,1'0:2"
#HISPEED 00
#00010:14
#00030A:14
#00134A:34
#00238A:24
)");
    const auto ugc = Parse(
        R"(@TICKS	480
@BPM	0'0	120
@TIL	0	0'0	0.5
@TIL	0	1'0	2
@ENDHEAD
#0'0:t04
#0'0:s04
#1920>s44
#3840>s84
)",
        CcFormat_Ugc
    );
    REQUIRE(sus);
    REQUIRE(ugc);
    CHECK(ugc->total_combo == sus->total_combo);
    REQUIRE(ugc->slider_holds.size() == 1);
    CHECK(ugc->slider_holds[0].points.size() == sus->slider_holds[0].points.size());
    CHECK(ugc->slider_holds[0].judge_points.size() == sus->slider_holds[0].judge_points.size());
    CHECK(ugc->slider_holds[0].points.back().sample == sus->slider_holds[0].points.back().sample);
    const auto& speed = ugc->timelines[ugc->slider_notes[0].timeline].speed_changes;
    CHECK(speed.back().sample == 960);
    CHECK(speed.back().multiplier == 2.0);
}

TEST_CASE("UGC global speed multiplies the selected timeline including real group zero", "[SheetsAnalyzer][IR]") {
    const auto result = Parse(
        R"(@TIL	0	0'0	0.5
@SPDMOD	0'0	2
@SPDMOD	1'0	-1
@ENDHEAD
#0'0:t04
)",
        CcFormat_Ugc
    );
    REQUIRE(result);
    REQUIRE(result->slider_notes.size() == 1);
    const auto& speed = result->timelines[result->slider_notes[0].timeline].speed_changes;
    REQUIRE(speed.size() >= 3);
    CHECK(speed[2].multiplier == 1.0);
    CHECK(speed.back().multiplier == -0.5);
    CHECK(result->timelines[0].speed_changes.back().multiplier == -1.0);
}

TEST_CASE("IR diagnostics refer to original source lines and reject information loss", "[SheetsAnalyzer][IR]") {
    for (const auto text : { "#TITLE \"x\"\n#00010:xx\n", "#TITLE \"x\"\n#00010:44\n" }) {
        const auto result = Parse(text);
        REQUIRE_FALSE(result);
        REQUIRE_FALSE(result.diagnostics.isEmpty());
        CHECK(result.diagnostics[0].path == U"chart.sus");
        CHECK(result.diagnostics[0].line == 2);
        CHECK_FALSE(result.diagnostics[0].message.contains(U"line 3:"));
    }
    CHECK_FALSE(Parse("@SPDDEF\t1\t0\t1.0\n@ENDHEAD\n", CcFormat_Ugc));
    CHECK_FALSE(Parse("#00030A:14\n"));
}

TEST_CASE("Source audio offsets are subtracted while metadata note offsets are added", "[SheetsAnalyzer][IR][Timing]") {
    const auto format = GENERATE(CcFormat_Sus, CcFormat_Ugc);
    const double source_offset = GENERATE(-0.5, 0.0, 1.25);
    const double metadata_offset = GENERATE(-0.25, 0.0, 0.25);
    CAPTURE(format, source_offset, metadata_offset);

    const auto source = std::string{ format == CcFormat_Sus ? "#WAVEOFFSET " : "@BGMOFS\t" } +
                        std::to_string(source_offset) + "\n" +
                        (format == CcFormat_Sus ? "#BPM01:120\n#00008:01\n#00010:00140000\n"
                                                : "@BPM\t0'0\t120\n@ENDHEAD\n#0'480:t04\n");
    const auto result = Parse(source, format, { .sample_rate = 480, .offset_seconds = metadata_offset });
    REQUIRE(result);
    REQUIRE(result->slider_notes.size() == 1);
    CHECK(result->slider_notes[0].sample == std::llround((0.5 + metadata_offset - source_offset) * 480));
}

TEST_CASE("Missing source audio offsets preserve metadata-only timing", "[SheetsAnalyzer][IR][Timing]") {
    const auto format = GENERATE(CcFormat_Sus, CcFormat_Ugc);
    const double metadata_offset = GENERATE(-0.25, 0.0, 0.25);
    const auto result = Parse(
        format == CcFormat_Sus ? "#00010:14\n" : "@ENDHEAD\n#0'0:t04\n",
        format,
        { .sample_rate = 480, .offset_seconds = metadata_offset }
    );
    REQUIRE(result);
    REQUIRE(result->slider_notes.size() == 1);
    CHECK(result->slider_notes[0].sample == std::llround(metadata_offset * 480));
}

TEST_CASE("IR audio offsets align notes, tempos, scrolls and hold judgements", "[SheetsAnalyzer][IR][Timing]") {
    const auto result = Parse(
        R"(@BGMOFS	1.25
@TICKS	480
@BPM	0'0	120
@BPM	0'480	180
@TIL	0	0'0	0.5
@TIL	0	0'480	1.5
@TIL	1	0'0	2
@SPDMOD	0'0	1
@ENDHEAD
#0'480:t04
#0'0:x04
#0'0:a04DRN
#0'0:s04
@USETIL	1
#960>s44
#1920>s84
#0'0:h22
#960>s22
#1920>s22
)",
        CcFormat_Ugc,
        { .sample_rate = 480, .offset_seconds = 0.25 }
    );
    REQUIRE(result);
    REQUIRE(result->slider_notes.size() == 1);
    REQUIRE(result->side_notes.size() == 1);
    CHECK(result->slider_notes[0].sample == -240);
    CHECK(result->side_notes[0].sample == -480);
    REQUIRE(result->tempo_changes.size() == 3);
    CHECK(result->tempo_changes[0].sample == -480);
    CHECK(result->tempo_changes[1].sample == -480);
    CHECK(result->tempo_changes[2].sample == -240);
    CHECK(result->tempo_changes[2].bpm == 180.0);
    for (const auto& timeline : result->timelines) {
        REQUIRE_FALSE(timeline.speed_changes.isEmpty());
        CHECK(timeline.speed_changes.front().sample == -480);
    }
    const auto& speed = result->timelines[result->slider_notes[0].timeline].speed_changes;
    CHECK(speed.back().sample == -240);
    CHECK(speed.back().multiplier == 1.5);

    REQUIRE(result->slider_holds.size() == 1);
    REQUIRE(result->side_holds.size() == 1);
    const auto& slider_hold = result->slider_holds[0];
    const auto& side_hold = result->side_holds[0];
    const s3d::Array<s3d::int64> point_samples{ -480, -80, 240 };
    const s3d::Array<s3d::int64> judge_samples{ -480, -360, -240, -160, -80, 0, 80, 160, 240 };
    CHECK(slider_hold.points.map([](const auto& point) {
        return point.sample;
    }) == point_samples);
    CHECK(side_hold.points.map([](const auto& point) {
        return point.sample;
    }) == point_samples);
    CHECK(slider_hold.judge_points.map([](const auto& point) {
        return point.sample;
    }) == judge_samples);
    CHECK(side_hold.judge_samples == judge_samples);
    CHECK(result->total_combo == 20);
}

TEST_CASE("IR timing rejects overflow when combining finite audio offsets", "[SheetsAnalyzer][IR][Timing]") {
    const auto format = GENERATE(CcFormat_Sus, CcFormat_Ugc);
    const auto result = Parse(
        format == CcFormat_Sus ? "#WAVEOFFSET -1e308\n#00010:14\n" : "@BGMOFS\t-1e308\n@ENDHEAD\n#0'0:t04\n",
        format,
        { .sample_rate = 480, .offset_seconds = 1e308 }
    );
    REQUIRE_FALSE(result);
    REQUIRE_FALSE(result.diagnostics.isEmpty());
    CHECK(result.diagnostics.front().severity == sheets::DiagnosticSeverity::Error);
}

TEST_CASE(
    "IR timing uses absolute beats, default BPM and stable simultaneous changes",
    "[SheetsAnalyzer][IR][Timing]"
) {
    const CcTempo tempos[]{ { { 2, 1 }, 60.0 }, { { 5, 1 }, 240.0 }, { { 5, 1 }, 180.0 } };
    const auto timing = compiler::TimingMap::Build(tempos, { .sample_rate = 120, .offset_seconds = 0.25 });
    REQUIRE(timing);
    CHECK(timing->toSample({ 0, 1 }) == 30);
    CHECK(timing->toSample({ 2, 1 }) == 150);
    CHECK(timing->toSample({ 5, 1 }) == 510);
    CHECK(timing->toSample({ 6, 1 }) == 550);
    CHECK_FALSE(compiler::TimingMap::Build({}, { .sample_rate = 0 }));
    CHECK_FALSE(compiler::TimingMap::Build({}, { .offset_seconds = std::numeric_limits<double>::infinity() }));
}

TEST_CASE("SUS curves are expanded only by ChartConverter without extra combo", "[SheetsAnalyzer][IR][Hold]") {
    for (const auto resolution : { 1, 480, 960 }) {
        const auto source = std::string{ "#REQUEST \"ticks_per_beat " } + std::to_string(resolution) +
                            "\"\n#00030A:12\n#0003DA:0042\n#00130A:22\n";
        const auto result = Parse(source, CcFormat_Sus, { .sample_rate = 100 });
        REQUIRE(result);
        REQUIRE(result->slider_holds.size() == 1);
        const auto& hold = result->slider_holds[0];
        REQUIRE(hold.points.size() == 41);
        REQUIRE(hold.judge_points.size() == 9);
        CHECK(hold.points[20].sample == 100);
        CHECK(hold.points[20].lane.start == 6);
        CHECK(hold.points[20].lane.width == 3);
        CHECK(hold.points[20].kind == sheets::SliderHoldPointKind::Invisible);
        CHECK(hold.judge_points[4].lane.start == 6);
        CHECK(result->total_combo == 9);
    }
}

TEST_CASE("Authored UGC controls stay on the path instead of becoming Bezier handles", "[SheetsAnalyzer][IR][Hold]") {
    const auto result = Parse(
        R"(@TICKS	480
@ENDHEAD
#0'0:s84
#40>c94
#104>cA4
#200>cB4
#360>cC4
#480>cC4
#600>cC4
#760>cB4
#856>cA4
#920>c94
#960>s84
)",
        CcFormat_Ugc,
        { .sample_rate = 960 }
    );
    REQUIRE(result);
    REQUIRE(result->slider_holds.size() == 1);
    const auto& hold = result->slider_holds[0];
    const s3d::Array<s3d::int64> expected_samples{ 0, 40, 104, 200, 360, 480, 600, 760, 856, 920, 960 };
    const s3d::Array<s3d::uint8> expected_lanes{ 8, 9, 10, 11, 12, 12, 12, 11, 10, 9, 8 };
    REQUIRE(hold.points.size() == expected_samples.size());
    for (std::size_t index = 0; index < hold.points.size(); ++index) {
        CHECK(hold.points[index].sample == expected_samples[index]);
        CHECK(hold.points[index].lane.start == expected_lanes[index]);
        CHECK(hold.points[index].lane.width == 4);
        if (index > 0 && index + 1 < hold.points.size()) {
            CHECK(hold.points[index].kind == sheets::SliderHoldPointKind::Invisible);
        }
    }
    REQUIRE(hold.judge_points.size() == 5);
    CHECK(hold.judge_points[2].sample == 480);
    CHECK(hold.judge_points[2].lane.start == 12);
    CHECK(hold.judge_points[2].lane.width == 4);
    CHECK(result->total_combo == 5);
}

TEST_CASE("IR path points retain their selected speed groups", "[SheetsAnalyzer][IR][Hold]") {
    const auto result = Parse(
        R"(@TIL	1	0'0	1.5
@TIL	2	0'0	2
@ENDHEAD
@USETIL	1
#0'0:s04
@USETIL	2
#480>c44
@USETIL	0
#960>s84
)",
        CcFormat_Ugc
    );
    REQUIRE(result);
    REQUIRE(result->slider_holds.size() == 1);
    const auto& points = result->slider_holds[0].points;
    REQUIRE(points.size() == 3);
    CHECK(points[1].kind == sheets::SliderHoldPointKind::Invisible);
    CHECK(result->timelines[points[0].timeline].speed_changes.back().multiplier == 1.5);
    CHECK(result->timelines[points[1].timeline].speed_changes.back().multiplier == 2.0);
    CHECK(result->timelines[points[2].timeline].speed_changes.back().multiplier == 1.0);
    CHECK(points[2].timeline != 0); // UGC group zero exists even without a TIL declaration.
}

TEST_CASE("SUS default requests preserve BOM encodings and diagnostic locations", "[SheetsAnalyzer][IR]") {
    for (const int encoding : { 0, 1, 2 }) {
        const std::string_view text = "#TITLE \"x\"\n#00010:xx\n";
        std::vector<s3d::uint8> bytes;
        if (encoding == 0) {
            bytes = { 0xEF, 0xBB, 0xBF };
        } else if (encoding == 1) {
            bytes = { 0xFF, 0xFE };
        } else {
            bytes = { 0xFE, 0xFF };
        }
        for (const auto c : text) {
            if (encoding == 2) {
                bytes.push_back(0);
            }
            bytes.push_back(static_cast<s3d::uint8>(c));
            if (encoding == 1) {
                bytes.push_back(0);
            }
        }
        const auto result = compiler::Parse(bytes, CcFormat_Sus, {}, U"encoded.sus");
        REQUIRE_FALSE(result);
        REQUIRE_FALSE(result.diagnostics.isEmpty());
        CHECK(result.diagnostics.front().line == 2);
        CHECK(result.diagnostics.front().path == U"encoded.sus");
    }
}

TEST_CASE("IR owners do not leak borrowed memory into concurrent chart loads", "[SheetsAnalyzer][IR]") {
    std::vector<std::future<sheets::Result<sheets::Chart>>> tasks;
    for (int i = 0; i < 8; ++i) {
        tasks.push_back(std::async(std::launch::async, [] {
            return Parse("#00010:24\n#00050:34\n#00038A:12\n#00138A:22\n");
        }));
    }
    for (auto& task : tasks) {
        const auto result = task.get();
        REQUIRE(result);
        CHECK(result->slider_notes.isEmpty());
        CHECK(result->side_notes.size() == 1);
        REQUIRE(result->slider_holds.size() == 1);
        CHECK(result->slider_holds[0].judge_points.size() == 9);
        CHECK(result->total_combo == 10);
    }
}

TEST_CASE("IR timing honors variable measures, tick requests and later tempo definitions", "[SheetsAnalyzer][IR]") {
    const auto result = Parse(R"(#REQUEST "ticks_per_beat 960"
#00002:3
#00102:5
#00210:14
#00108:01
#BPM01:60
#TIL00:"1'960:2"
#HISPEED 00
#00114:14
)");
    REQUIRE(result);
    REQUIRE(result->slider_notes.size() == 2);
    CHECK(result->slider_notes[0].sample == 720);
    CHECK(result->slider_notes[1].sample == 3120);
    CHECK(result->timelines[result->slider_notes[0].timeline].speed_changes.back().sample == 1200);
}
