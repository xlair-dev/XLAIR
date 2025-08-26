#pragma once
#define NO_S3D_USING
#include <Siv3D.hpp>
#include "Metadata.hpp" 

namespace SheetsAnalyzer {
    s3d::Array<Metadata> AnalyzeAll(const s3d::FilePath& directory);

    s3d::Optional<Metadata> Analyze(const s3d::FilePath& path);
}
