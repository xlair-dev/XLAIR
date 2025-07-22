#pragma once
#define NO_S3D_USING
#include <Siv3D.hpp>
#include "SUSAnalyzer/SUSData.hpp"

namespace SheetsAnalyzer::SUSAnalyzer {
    s3d::Optional<SUSData> Analyze(const s3d::FilePath& path);
}
