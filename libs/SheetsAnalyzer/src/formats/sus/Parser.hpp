#pragma once

#include <Siv3D.hpp>

#include "SheetsAnalyzer/Result.hpp"
#include "Document.hpp"

namespace xlair::sheets::formats::sus {
    [[nodiscard]]
    Result<Document> Parse(const s3d::FilePath& path);

    [[nodiscard]]
    Result<Document> ParseText(s3d::StringView text, const s3d::FilePath& path = U"");
}
