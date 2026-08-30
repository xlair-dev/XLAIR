#pragma once
#define NO_S3D_USING

#include <Siv3D.hpp>
#include "SheetsAnalyzer/Result.hpp"
#include "SheetsAnalyzer/Metadata.hpp"

namespace xlair::sheets::metadata {
    [[nodiscard]] Result<Metadata> Load(const s3d::FilePath& path);

    [[nodiscard]] Result<s3d::Array<Metadata>> Scan(const s3d::FilePath& directory);
}
