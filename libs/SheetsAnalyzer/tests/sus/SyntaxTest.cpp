#include <ThirdParty/Catch2/catch.hpp>

#include "formats/sus/Syntax.hpp"

#include <variant>

namespace sus = xlair::sheets::formats::sus;

TEST_CASE("ParseLine ignores lines that are not SUS directives", "[SheetsAnalyzer][SUS][Syntax]") {
    const auto empty = sus::ParseLine(U"", 1, U"chart.sus");
    const auto comment = sus::ParseLine(U"This is a comment", 2, U"chart.sus");
    const auto indented = sus::ParseLine(U"  #TITLE \"Ignored\"", 3, U"chart.sus");

    REQUIRE(empty);
    REQUIRE(comment);
    REQUIRE(indented);
    CHECK(std::holds_alternative<sus::IgnoredLine>(*empty));
    CHECK(std::holds_alternative<sus::IgnoredLine>(*comment));
    CHECK(std::holds_alternative<sus::IgnoredLine>(*indented));
}

TEST_CASE("ParseLine parses SUS commands", "[SheetsAnalyzer][SUS][Syntax]") {
    SECTION("quoted argument") {
        const auto result = sus::ParseLine(U"#title \"Song Title\"", 4, U"chart.sus");

        REQUIRE(result);
        REQUIRE(std::holds_alternative<sus::CommandLine>(*result));

        const auto& command = std::get<sus::CommandLine>(*result);
        CHECK(command.key == U"TITLE");
        CHECK(command.argument == U"\"Song Title\"");
    }

    SECTION("colon-separated definition") {
        const auto result = sus::ParseLine(U"#BPM0a: 140.0", 5, U"chart.sus");

        REQUIRE(result);
        const auto& command = std::get<sus::CommandLine>(*result);
        CHECK(command.key == U"BPM0A");
        CHECK(command.argument == U"140.0");
    }

    SECTION("argument-less command") {
        const auto result = sus::ParseLine(U"#NOSPEED", 6, U"chart.sus");

        REQUIRE(result);
        const auto& command = std::get<sus::CommandLine>(*result);
        CHECK(command.key == U"NOSPEED");
        CHECK(command.argument.isEmpty());
    }
}

TEST_CASE("ParseLine parses SUS data lines without interpreting their data code", "[SheetsAnalyzer][SUS][Syntax]") {
    const auto result = sus::ParseLine(U"\uFEFF#01220A: 14002400\r", 7, U"chart.sus");

    REQUIRE(result);
    REQUIRE(std::holds_alternative<sus::DataLine>(*result));

    const auto& data = std::get<sus::DataLine>(*result);
    CHECK(data.measure == 12);
    CHECK(data.code == U"20a");
    CHECK(data.data == U"14002400");
    CHECK(data.data_column == 10);
}

TEST_CASE("ParseLine reports malformed SUS lines with source locations", "[SheetsAnalyzer][SUS][Syntax]") {
    SECTION("missing directive") {
        const auto result = sus::ParseLine(U"#", 8, U"broken.sus");

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().path == U"broken.sus");
        CHECK(*result.diagnostics.front().line == 8);
        CHECK(*result.diagnostics.front().column == 2);
    }

    SECTION("missing data separator") {
        const auto result = sus::ParseLine(U"#00111 0100", 9, U"broken.sus");

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().message == U"Data lines require a ':' separator.");
    }

    SECTION("missing data code") {
        const auto result = sus::ParseLine(U"#001:0100", 10, U"broken.sus");

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().message ==
              U"Data line headers require at least a two-character data code after the measure.");
    }

    SECTION("missing data") {
        const auto result = sus::ParseLine(U"#00111:", 11, U"broken.sus");

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().message == U"Data lines require a value after ':'.");
    }
}

TEST_CASE("ParseBase36 accepts both letter cases", "[SheetsAnalyzer][SUS][Syntax]") {
    const auto zero = sus::ParseBase36(U"0", 1, 1);
    const auto lowercase = sus::ParseBase36(U"z", 1, 1);
    const auto pair = sus::ParseBase36(U"ZZ", 1, 1);

    REQUIRE(zero);
    REQUIRE(lowercase);
    REQUIRE(pair);
    CHECK(*zero == 0);
    CHECK(*lowercase == 35);
    CHECK(*pair == 1295);
}

TEST_CASE("ParseBase36 reports invalid digits and overflow", "[SheetsAnalyzer][SUS][Syntax]") {
    SECTION("invalid digit") {
        const auto result = sus::ParseBase36(U"1!", 12, 7, U"broken.sus");

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(*result.diagnostics.front().line == 12);
        CHECK(*result.diagnostics.front().column == 8);
    }

    SECTION("overflow") {
        const auto result = sus::ParseBase36(U"ZZZZZZZ", 13, 3, U"broken.sus");

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().message == U"Base36 value exceeds the uint32 range.");
    }
}
