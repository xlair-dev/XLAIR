#pragma once
#define NO_S3D_USING
#include <Siv3D.hpp>

#include "SheetData.hpp"

namespace SheetsAnalyzer::SUSAnalyzer {
    s3d::Optional<SheetData> Analyze(const s3d::FilePath& path, const s3d::int64 sample_rate, const double offset_sec);
}
