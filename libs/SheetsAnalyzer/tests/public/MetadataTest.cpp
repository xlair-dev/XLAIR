#include <ThirdParty/Catch2/catch.hpp>

#include <SheetsAnalyzer.hpp>

namespace {
    [[nodiscard]]
    s3d::FilePath MetadataFixture(const s3d::StringView path) {
        return XLAIR_SHEETS_ANALYZER_TEST_FIXTURE_DIR U"metadata/" + path;
    }

    [[nodiscard]]
    std::string FirstDiagnostic(const xlair::sheets::Result<xlair::sheets::Metadata>& result) {
        return result.diagnostics.isEmpty() ? std::string{} : result.diagnostics.front().message.narrow();
    }
}

TEST_CASE("Metadata has stable defaults", "[SheetsAnalyzer][Metadata]") {
    const xlair::sheets::Metadata metadata;

    CHECK(metadata.title == U"Untitled");
    CHECK(metadata.title_sort == U"Untitled");
    CHECK(metadata.artist == U"Unknown Artist");
    CHECK(metadata.genre == U"Unspecified");
    CHECK(metadata.bpm == 120.0);
    CHECK(metadata.difficulties.isEmpty());
}

TEST_CASE("LoadMetadata reads JSON metadata and resolves asset paths", "[SheetsAnalyzer][Metadata][JSON]") {
    const auto path = MetadataFixture(U"load/music.json");
    const auto result = xlair::sheets::LoadMetadata(path);

    INFO(FirstDiagnostic(result));
    REQUIRE(result);
    CHECK(result->source_path == s3d::FileSystem::FullPath(path));
    CHECK(result->id == U"json-song");
    CHECK(result->title == U"JSON Song");
    CHECK(result->title_sort == U"json song");
    CHECK(result->artist == U"JSON Artist");
    CHECK(result->genre == U"Test");
    CHECK(result->music == s3d::FileSystem::FullPath(MetadataFixture(U"load/audio/song.wav")));
    CHECK(
        result->jacket == s3d::FileSystem::FullPath(
                              s3d::FileSystem::PathAppend(s3d::FileSystem::CurrentDirectory(), U"shared/jacket.png")
                          )
    );
    CHECK(result->url == U"https://example.com/json-song");
    CHECK(result->music_offset_seconds == -0.125);
    CHECK(result->demo_start_seconds == 12.5);
    CHECK(result->bpm == 175.0);

    REQUIRE(result->difficulties.size() == 2);
    CHECK(result->difficulties[0].index == 0);
    CHECK(result->difficulties[0].id == U"basic");
    CHECK(result->difficulties[0].src == s3d::FileSystem::FullPath(MetadataFixture(U"load/charts/basic.sus")));
    CHECK(result->difficulties[1].index == 2);
    CHECK(result->difficulties[1].id == U"expert");
    CHECK(result->difficulties[1].designer == U"Chart Author");
}

TEST_CASE("LoadMetadata reads TOML metadata and version 1 difficulty keys", "[SheetsAnalyzer][Metadata][TOML]") {
    const auto result = xlair::sheets::LoadMetadata(MetadataFixture(U"load/music.toml"));

    INFO(FirstDiagnostic(result));
    REQUIRE(result);
    CHECK(result->id == U"toml-song");
    CHECK(result->title == U"TOML Song");
    CHECK(result->artist == U"Unknown Artist");
    CHECK(result->bpm == 120.0);
    REQUIRE(result->difficulties.size() == 1);
    CHECK(result->difficulties.front().index == 1);
    CHECK(result->difficulties.front().id == U"advanced");
    CHECK(result->difficulties.front().src == s3d::FileSystem::FullPath(MetadataFixture(U"load/charts/advanced.sus")));
}

TEST_CASE("LoadMetadata applies version 1 difficulty indices", "[SheetsAnalyzer][Metadata]") {
    const auto result = xlair::sheets::LoadMetadata(MetadataFixture(U"load/difficulties.json"));

    INFO(FirstDiagnostic(result));
    REQUIRE(result);
    REQUIRE(result->difficulties.size() == 3);
    CHECK(result->difficulties[0].index == 0);
    CHECK(result->difficulties[0].id == U"default-index");
    CHECK(result->difficulties[1].index == 1);
    CHECK(result->difficulties[1].id == U"replacement");
    CHECK(result->difficulties[1].src == s3d::FileSystem::FullPath(MetadataFixture(U"load/charts/replacement.sus")));
    CHECK(result->difficulties[2].index == 10);
    CHECK(result->difficulties[2].id == U"high-index");
}

TEST_CASE("LoadMetadata ignores a non-array difficulties value", "[SheetsAnalyzer][Metadata]") {
    const auto result = xlair::sheets::LoadMetadata(MetadataFixture(U"load/non-array.json"));

    INFO(FirstDiagnostic(result));
    REQUIRE(result);
    CHECK(result->difficulties.isEmpty());
}

TEST_CASE("LoadMetadata reports unsupported and invalid metadata", "[SheetsAnalyzer][Metadata]") {
    SECTION("unsupported extension") {
        const auto result = xlair::sheets::LoadMetadata(U"music.yaml");

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().message == U"Unsupported metadata format: yaml");
    }

    SECTION("unsupported version") {
        const auto result = xlair::sheets::LoadMetadata(MetadataFixture(U"invalid/version.json"));

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().message == U"Metadata must use supported version 1.");
    }

    SECTION("invalid BPM") {
        const auto result = xlair::sheets::LoadMetadata(MetadataFixture(U"invalid/bpm.toml"));

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().message == U"Metadata BPM must be a positive finite number.");
    }

    SECTION("invalid difficulty level") {
        const auto result = xlair::sheets::LoadMetadata(MetadataFixture(U"invalid/level.json"));

        REQUIRE_FALSE(result);
        REQUIRE(result.diagnostics.size() == 1);
        CHECK(result.diagnostics.front().message == U"Difficulty levels must be non-negative finite numbers.");
    }
}

TEST_CASE("ScanMetadata recursively loads conventionally named metadata", "[SheetsAnalyzer][Metadata][Scan]") {
    const auto result = xlair::sheets::ScanMetadata(MetadataFixture(U"scan"));

    REQUIRE(result);
    REQUIRE(result->size() == 2);
    CHECK((*result)[0].id == U"alpha");
    CHECK((*result)[1].id == U"beta");
}

TEST_CASE("ScanMetadata rejects a non-directory path", "[SheetsAnalyzer][Metadata][Scan]") {
    const auto path = MetadataFixture(U"load/music.json");
    const auto result = xlair::sheets::ScanMetadata(path);

    REQUIRE_FALSE(result);
    REQUIRE(result.diagnostics.size() == 1);
    CHECK(result.diagnostics.front().path == path);
    CHECK(result.diagnostics.front().message == U"Metadata scan path must be a directory.");
}

TEST_CASE("ScanMetadata reports invalid metadata found during traversal", "[SheetsAnalyzer][Metadata][Scan]") {
    const auto result = xlair::sheets::ScanMetadata(MetadataFixture(U"scan-invalid"));

    REQUIRE_FALSE(result);
    REQUIRE(result.diagnostics.size() == 1);
    CHECK(result.diagnostics.front().message == U"Metadata must use supported version 1.");
    CHECK(result.diagnostics.front().path.ends_with(U"broken/music.toml"));
}

TEST_CASE("Chart load options use the engine sample rate", "[SheetsAnalyzer][Chart]") {
    const xlair::sheets::ChartLoadOptions options;

    CHECK(options.sample_rate == 44'100);
    CHECK(options.offset_seconds == 0.0);
}
