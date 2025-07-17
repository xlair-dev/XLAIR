#pragma once
#include "SheetsAnalyzer.hpp"

namespace SheetsAnalyzer::internal {
    s3d::Optional<Metadata> AnalyzeJsonFile(const s3d::FilePath& filePath);
    s3d::Optional<Metadata> AnalyzeTomlFile(const s3d::FilePath& filePath);
}
