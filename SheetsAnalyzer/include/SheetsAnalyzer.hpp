#pragma once
#define NO_S3D_USING
#include <Siv3D.hpp>
#include "Metadata.hpp"
#include "SheetData.hpp"

namespace SheetsAnalyzer {
    s3d::Array<Metadata> AnalyzeAll(const s3d::FilePath& directory);

    s3d::Optional<Metadata> Analyze(const s3d::FilePath& path);

    s3d::Optional<SheetData> AnalyzeData(const s3d::FilePath& path, const s3d::int64 sample_rate, const double offset_sec);
}
