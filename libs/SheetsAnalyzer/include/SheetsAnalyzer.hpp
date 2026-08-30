#pragma once
#define NO_S3D_USING

#include <Siv3D.hpp>

#include "SheetsAnalyzer/Chart.hpp"
#include "SheetsAnalyzer/Metadata.hpp"
#include "SheetsAnalyzer/Result.hpp"

namespace xlair::sheets {
    struct ChartLoadOptions {
        s3d::int64 sample_rate = 44'100;
        double offset_seconds = 0.0;
    };

    // Loads one song metadata file (currently JSON / TOML).
    [[nodiscard]] Result<Metadata> LoadMetadata(const s3d::FilePath& path);

    // Recursively searches a directory for supported metadata files.
    [[nodiscard]] Result<s3d::Array<Metadata>> ScanMetadata(const s3d::FilePath& directory);

    // Loads and compiles a chart into XLAIR's input-oriented Chart model.
    // supported formats:
    // - SUS
    [[nodiscard]] Result<Chart> LoadChart(const s3d::FilePath& path, const ChartLoadOptions& options = {});
}
