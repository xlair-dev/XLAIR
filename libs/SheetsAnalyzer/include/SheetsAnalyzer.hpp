#pragma once

#include <Siv3D.hpp>

#include "SheetsAnalyzer/Chart.hpp"
#include "SheetsAnalyzer/Metadata.hpp"
#include "SheetsAnalyzer/Result.hpp"

namespace xlair::sheets {
    struct ChartLoadOptions {
        s3d::int64 sample_rate = 44'100;
        /// Game offset from song metadata; source WAVEOFFSET is not added to it.
        double offset_seconds = 0.0;
    };

    // Loads one song metadata file (currently JSON / TOML).
    [[nodiscard]]
    Result<Metadata> LoadMetadata(const s3d::FilePath& path);

    // Recursively searches a directory for music.json and music.toml files.
    [[nodiscard]]
    Result<s3d::Array<Metadata>>
    ScanMetadata(const s3d::FilePath& directory, const s3d::Array<s3d::FilePath>& excluded_directories = {});

    // Parses with ChartConverter's XLAIR mode and compiles into the game's input-oriented model.
    // supported formats:
    // - SUS
    [[nodiscard]]
    Result<Chart> LoadChart(const s3d::FilePath& path, const ChartLoadOptions& options = {});
}
