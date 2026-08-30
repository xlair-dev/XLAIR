#include "Syntax.hpp"

#include <Siv3D/Char.hpp>

namespace xlair::sheets::formats::sus {
    Result<ParsedLine> ParseLine(const s3d::StringView line, const std::size_t line_number, const s3d::FilePath& path) {
        s3d::StringView source = line;

        if (!source.isEmpty() && source.front() == U'\uFEFF') {
            source = source.substr(1);
        }

        if (source.isEmpty() || source.front() != U'#') {
            Result<ParsedLine> result;
            result.value = ParsedLine{ IgnoredLine{} };
            return result;
        }

        const s3d::String body = s3d::String{ source.substr(1) }.rtrimmed();
        if (body.isEmpty()) {
            return Result<ParsedLine>::makeError(U"Expected a SUS directive after '#'.", path, line_number, 2);
        }

        if (s3d::IsDigit(body.front())) {
            if (body.size() < 3 || !s3d::IsDigit(body[1]) || !s3d::IsDigit(body[2])) {
                return Result<ParsedLine>::makeError(U"Data line measures must contain exactly three decimal digits.",
                                                     path, line_number, 2);
            }

            const std::size_t separator = body.indexOf(U':');
            if (separator == s3d::String::npos) {
                return Result<ParsedLine>::makeError(U"Data lines require a ':' separator.", path, line_number,
                                                     body.size() + 2);
            }

            if (separator < 5) {
                return Result<ParsedLine>::makeError(
                    U"Data line headers require at least a two-character data code after the measure.", path,
                    line_number, separator + 2);
            }

            const s3d::String code = body.substr(3, separator - 3).lowercased();
            for (std::size_t index = 0; index < code.size(); ++index) {
                if (!s3d::IsAlnum(code[index])) {
                    return Result<ParsedLine>::makeError(U"Data line codes must use ASCII alphanumeric characters.",
                                                         path, line_number, index + 5);
                }
            }

            const s3d::String data = body.substr(separator + 1).trimmed();
            if (data.isEmpty()) {
                return Result<ParsedLine>::makeError(U"Data lines require a value after ':'.", path, line_number,
                                                     separator + 3);
            }

            const s3d::uint32 measure =
                static_cast<s3d::uint32>((body[0] - U'0') * 100 + (body[1] - U'0') * 10 + (body[2] - U'0'));

            Result<ParsedLine> result;
            result.value = ParsedLine{ DataLine{
                .measure = measure,
                .code = code,
                .data = data,
            } };
            return result;
        }

        std::size_t separator = s3d::String::npos;
        for (std::size_t index = 0; index < body.size(); ++index) {
            if (body[index] == U':' || s3d::IsSpace(body[index])) {
                separator = index;
                break;
            }
        }

        const s3d::String key = body.substr(0, separator).uppercased();
        if (key.isEmpty()) {
            return Result<ParsedLine>::makeError(U"Expected a SUS directive name after '#'.", path, line_number, 2);
        }

        for (std::size_t index = 0; index < key.size(); ++index) {
            if (!s3d::IsAlnum(key[index])) {
                return Result<ParsedLine>::makeError(U"SUS directive names must use ASCII alphanumeric characters.",
                                                     path, line_number, index + 2);
            }
        }

        s3d::String argument;
        if (separator != s3d::String::npos) {
            argument = body.substr(separator + 1).trimmed();
        }

        Result<ParsedLine> result;
        result.value = ParsedLine{ CommandLine{
            .key = key,
            .argument = argument,
        } };
        return result;
    }

    Result<s3d::uint32> ParseBase36(const s3d::StringView value, const std::size_t line_number,
                                    const std::size_t column, const s3d::FilePath& path) {
        if (value.isEmpty()) {
            return Result<s3d::uint32>::makeError(U"Base36 values must not be empty.", path, line_number, column);
        }

        for (std::size_t index = 0; index < value.size(); ++index) {
            if (!s3d::IsAlnum(value[index])) {
                return Result<s3d::uint32>::makeError(U"Base36 values may contain only 0-9 and A-Z.", path, line_number,
                                                      column + index);
            }
        }

        const auto parsed = s3d::ParseIntOpt<s3d::uint32>(value, s3d::Arg::radix = 36);
        if (!parsed) {
            return Result<s3d::uint32>::makeError(U"Base36 value exceeds the uint32 range.", path, line_number, column);
        }

        return { .value = *parsed };
    }
}
