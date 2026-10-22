#include "Loader.hpp"
#include "Compiler.hpp"

#include <memory>
#include <string_view>
#include <vector>

namespace xlair::sheets::chart {
    namespace {
        std::vector<s3d::uint8> WithSusDefaults(const std::span<const s3d::uint8> source) {
            // A default only: an explicit request in the file wins. No source-format
            // parsing or SUS-to-SUS conversion is performed by this boundary.
            constexpr std::string_view directive = "#REQUEST \"xlair_directional_mode consume_tap\"\n";
            const bool little = source.size() >= 2 && source[0] == 0xFF && source[1] == 0xFE;
            const bool big = source.size() >= 2 && source[0] == 0xFE && source[1] == 0xFF;
            const std::size_t bom =
                little || big
                    ? 2
                    : (source.size() >= 3 && source[0] == 0xEF && source[1] == 0xBB && source[2] == 0xBF ? 3 : 0);
            std::vector<s3d::uint8> bytes;
            bytes.reserve(source.size() + directive.size() * (little || big ? 2 : 1));
            bytes.insert(bytes.end(), source.begin(), source.begin() + bom);
            for (const auto character : directive) {
                if (big) {
                    bytes.push_back(0);
                }
                bytes.push_back(static_cast<s3d::uint8>(character));
                if (little) {
                    bytes.push_back(0);
                }
            }
            bytes.insert(bytes.end(), source.begin() + bom, source.end());
            return bytes;
        }
    }

    Result<Chart> Parse(
        std::span<const s3d::uint8> source,
        const CcFormat format,
        const ChartLoadOptions& options,
        const s3d::FilePath& path
    ) {
        if (cc_api_version() != CC_API_VERSION) {
            return Result<Chart>::makeError(U"ChartConverter header/library version mismatch.", path);
        }
        const auto bytes = format == CcFormat_Sus ? WithSusDefaults(source) : std::vector<s3d::uint8>{};
        if (format == CcFormat_Sus) {
            source = bytes;
        }
        const std::unique_ptr<CcResult, decltype(&cc_result_free)> parsed{
            cc_parse(format, CcMode_Xlair, source.data(), source.size()),
            &cc_result_free
        };
        if (!parsed) {
            return Result<Chart>::makeError(U"ChartConverter did not return a result.", path);
        }
        Result<Chart> result;
        const auto diagnostics = cc_result_diagnostics(parsed.get());
        bool omitted = false;
        for (const auto& diagnostic : std::span{ diagnostics.diagnostics, diagnostics.count }) {
            const std::size_t prefix_lines = format == CcFormat_Sus ? 1 : 0;
            auto message = s3d::Unicode::FromUTF8(diagnostic.message);
            if (prefix_lines && diagnostic.line > prefix_lines) {
                // The C API also embeds the line in fatal parser error text.
                // Adjust only that generated prefix, never user-supplied values.
                const auto prefix = s3d::Format(U"failed to parse SUS chart: line ", diagnostic.line, U": ");
                if (message.starts_with(prefix)) {
                    message = s3d::Format(U"failed to parse SUS chart: line ", diagnostic.line - prefix_lines, U": ") +
                              message.substr(prefix.size());
                }
            }
            result.diagnostics.push_back(
                {
                    .severity = diagnostic.severity == CcDiagnosticSeverity_Error ? DiagnosticSeverity::Error
                                                                                  : DiagnosticSeverity::Warning,
                    .message = std::move(message),
                    .path = path,
                    .line = diagnostic.line > prefix_lines
                                ? s3d::Optional<std::size_t>{ diagnostic.line - prefix_lines }
                                : s3d::none,
                }
            );
            omitted |= diagnostic.severity == CcDiagnosticSeverity_Warning;
        }
        if (cc_result_error(parsed.get())) {
            return result;
        }
        // Current parser warnings describe information loss. Never start a game with
        // notes silently removed by a format frontend.
        if (omitted) {
            result.diagnostics.push_back(
                { .severity = DiagnosticSeverity::Error,
                  .message = U"The chart contains unsupported notation; refusing to load an incomplete chart.",
                  .path = path }
            );
            return result;
        }
        auto compiled = Compile(*parsed, options);
        for (auto& diagnostic : compiled.diagnostics) {
            if (diagnostic.path.isEmpty()) {
                diagnostic.path = path;
            }
            result.diagnostics.push_back(std::move(diagnostic));
        }
        result.value = std::move(compiled.value);
        return result;
    }

    Result<Chart> Load(const s3d::FilePath& path, const ChartLoadOptions& options) {
        const auto extension = s3d::FileSystem::Extension(path).lowercased();
        if (extension != U"sus" && extension != U"ugc") {
            return Result<Chart>::makeError(U"Unsupported chart format: " + extension, path);
        }
        s3d::Blob bytes;
        if (!bytes.createFromFile(path)) {
            return Result<Chart>::makeError(U"Failed to read the chart file.", path);
        }
        return Parse(
            { reinterpret_cast<const s3d::uint8*>(bytes.data()), bytes.size() },
            extension == U"sus" ? CcFormat_Sus : CcFormat_Ugc,
            options,
            path
        );
    }
}
