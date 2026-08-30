#include "SheetsAnalyzer.hpp"

#include "formats/sus/Compiler.hpp"
#include "formats/sus/Parser.hpp"
#include "metadata/Loader.hpp"

namespace xlair::sheets {
    namespace {
        void AppendDiagnostics(s3d::Array<Diagnostic>& destination, const s3d::Array<Diagnostic>& source) {
            for (const auto& diagnostic : source) {
                destination.push_back(diagnostic);
            }
        }
    }

    Result<Metadata> LoadMetadata(const s3d::FilePath& path) {
        return metadata::Load(path);
    }

    Result<s3d::Array<Metadata>> ScanMetadata(const s3d::FilePath& directory) {
        return metadata::Scan(directory);
    }

    Result<Chart> LoadChart(const s3d::FilePath& path, const ChartLoadOptions& options) {
        Result<Chart> result;

        const auto extension = s3d::FileSystem::Extension(path).lowercased();
        if (extension != U"sus") {
            result.diagnostics.push_back({
                .severity = DiagnosticSeverity::Error,
                .message = U"Unsupported chart format: " + extension,
                .path = path,
                .line = s3d::none,
                .column = s3d::none,
            });
            return result;
        }

        auto parsed = formats::sus::Parse(path);
        AppendDiagnostics(result.diagnostics, parsed.diagnostics);
        if (!parsed) {
            return result;
        }

        auto compiled = formats::sus::Compile(*parsed, {
                                                           .sample_rate = options.sample_rate,
                                                           .offset_seconds = options.offset_seconds,
                                                       });

        AppendDiagnostics(result.diagnostics, compiled.diagnostics);
        if (compiled) {
            result.value = std::move(compiled.value);
        }

        return result;
    }
}
