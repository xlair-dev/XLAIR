#include <ThirdParty/Catch2/catch.hpp>

#include <SheetsAnalyzer.hpp>

namespace {
    [[nodiscard]]
    s3d::FilePath SusFixture(const s3d::StringView name) {
        return XLAIR_SHEETS_ANALYZER_TEST_FIXTURE_DIR U"sus/" + name;
    }

    [[nodiscard]]
    std::string FirstDiagnostic(const xlair::sheets::Result<xlair::sheets::Chart>& result) {
        return result.diagnostics.isEmpty() ? std::string{} : result.diagnostics.front().message.narrow();
    }
}

TEST_CASE("LoadChart rejects unsupported formats", "[SheetsAnalyzer][Chart]") {
    const auto result = xlair::sheets::LoadChart(U"chart.txt");

    REQUIRE_FALSE(result);
    REQUIRE(result.diagnostics.size() == 1);
    CHECK(result.diagnostics.front().severity == xlair::sheets::DiagnosticSeverity::Error);
    CHECK(result.diagnostics.front().path == U"chart.txt");
}

TEST_CASE("LoadChart compiles SUS fixtures end to end", "[SheetsAnalyzer][Chart][SUS][E2E]") {
    constexpr xlair::sheets::ChartLoadOptions options{
        .sample_rate = 480,
    };

    SECTION("short notes") {
        const auto result = xlair::sheets::LoadChart(SusFixture(U"0.sus"), options);

        INFO(FirstDiagnostic(result));
        REQUIRE(result);
        CHECK(result->slider_notes.size() == 72);
        CHECK(result->slider_holds.isEmpty());
        CHECK(result->timelines.size() == 2);
        CHECK(result->total_combo == 72);
    }

    SECTION("hispeed changes") {
        const auto result = xlair::sheets::LoadChart(SusFixture(U"hispeed.sus"), options);

        INFO(FirstDiagnostic(result));
        REQUIRE(result);
        REQUIRE(result->timelines.size() == 3);
        REQUIRE(result->timelines[1].speed_changes.size() == 3);
        CHECK(result->timelines[1].speed_changes[1].multiplier == 0.5);
        CHECK(result->timelines[1].speed_changes[2].sample == 2'880);
        CHECK(result->timelines[1].speed_changes[2].multiplier == 2.0);
        REQUIRE(result->timelines[2].speed_changes.size() == 3);
        CHECK(result->timelines[2].speed_changes[1].multiplier == 2.0);
        CHECK(result->timelines[2].speed_changes[2].sample == 2'880);
        CHECK(result->timelines[2].speed_changes[2].multiplier == 0.5);
        REQUIRE(result->slider_notes.size() == 15);
        CHECK(result->slider_notes[0].timeline == 1);
        CHECK(result->slider_notes[1].timeline == 2);
        CHECK(result->slider_notes[2].timeline == 0);
        CHECK(result->total_combo == 15);
    }

    SECTION("slider hold") {
        const auto result = xlair::sheets::LoadChart(SusFixture(U"hold.sus"), options);

        INFO(FirstDiagnostic(result));
        REQUIRE(result);
        REQUIRE(result->slider_holds.size() == 1);
        const auto& hold = result->slider_holds.front();
        REQUIRE(hold.points.size() == 3);
        CHECK(hold.points.front().sample == 960);
        CHECK(hold.points.back().sample == 1'920);
        CHECK(hold.judge_points.size() == 9);
        CHECK(result->total_combo == 9);
    }

    SECTION("side notes and hold") {
        const auto result = xlair::sheets::LoadChart(SusFixture(U"side.sus"), options);

        INFO(FirstDiagnostic(result));
        REQUIRE(result);
        REQUIRE(result->side_notes.size() == 2);
        CHECK(result->side_notes[0].button == xlair::sheets::SideButton::LeftUpper);
        CHECK(result->side_notes[1].button == xlair::sheets::SideButton::RightLower);
        REQUIRE(result->side_holds.size() == 1);
        const auto& hold = result->side_holds.front();
        CHECK(hold.button == xlair::sheets::SideButton::LeftUpper);
        REQUIRE(hold.points.size() == 3);
        CHECK(hold.points.front().sample == 960);
        CHECK(hold.points.back().sample == 2'880);
        CHECK(hold.judge_samples.size() == 17);
        CHECK(result->total_combo == 19);
    }
}
