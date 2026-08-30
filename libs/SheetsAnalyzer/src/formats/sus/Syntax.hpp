#pragma once

#include <Siv3D.hpp>

#include "SheetsAnalyzer/Result.hpp"

#include <cstddef>
#include <variant>

namespace xlair::sheets::formats::sus {
    struct IgnoredLine {};

    struct CommandLine {
        s3d::String key;
        s3d::String argument;
    };

    struct DataLine {
        s3d::uint32 measure = 0;
        s3d::String channel;
        s3d::String data;
    };

    using ParsedLine = std::variant<IgnoredLine, CommandLine, DataLine>;

    [[nodiscard]] Result<ParsedLine> ParseLine(s3d::StringView line, std::size_t line_number,
                                               const s3d::FilePath& path = U"");

    [[nodiscard]] Result<s3d::uint32> ParseBase36(s3d::StringView value, std::size_t line_number, std::size_t column,
                                                  const s3d::FilePath& path = U"");
}
