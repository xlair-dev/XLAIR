#pragma once

#include <Siv3D.hpp>

#include "SheetsAnalyzer/Chart.hpp"
#include "SheetsAnalyzer/Result.hpp"
#include "Document.hpp"

namespace xlair::sheets::formats::sus {
    struct CompileOptions {
        s3d::int64 sample_rate = 44'100;
        double offset_seconds = 0.0;
    };

    namespace detail {
        [[nodiscard]]
        s3d::Optional<SideButton> SideButtonFromSideLongLane(s3d::uint8 lane);
    }

    [[nodiscard]]
    Result<Chart> Compile(const Document& document, const CompileOptions& options);
}
