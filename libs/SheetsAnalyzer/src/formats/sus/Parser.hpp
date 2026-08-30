#pragma once
#define NO_S3D_USING

#include <Siv3D.hpp>

#include "SheetsAnalyzer/Result.hpp"
#include "Document.hpp"

namespace xlair::sheets::formats::sus {
    [[nodiscard]] Result<Document> Parse(const s3d::FilePath& path);
}
