#pragma once

#include <chart_converter.h>
#include "SheetsAnalyzer.hpp"

#include <span>

namespace xlair::sheets::chart {
    [[nodiscard]]
    Result<Chart> Load(const s3d::FilePath& path, const ChartLoadOptions& options);

    // Byte-oriented boundary shared by file loading and headless integration tests.
    [[nodiscard]]
    Result<Chart> Parse(
        std::span<const s3d::uint8> source,
        CcFormat format,
        const ChartLoadOptions& options = {},
        const s3d::FilePath& path = U""
    );
}
