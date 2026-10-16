#include "SheetsAnalyzer.hpp"

#include "compilation/Compiler.hpp"
#include "metadata/Loader.hpp"

#include <memory>
#include <utility>

namespace xlair::sheets {
    Result<Metadata> LoadMetadata(const s3d::FilePath& path) {
        return metadata::Load(path);
    }

    Result<s3d::Array<Metadata>>
    ScanMetadata(const s3d::FilePath& directory, const s3d::Array<s3d::FilePath>& excluded_directories) {
        return metadata::Scan(directory, excluded_directories);
    }

    Result<Chart> LoadChart(const s3d::FilePath& path, const ChartLoadOptions& options) {
        const auto extension = s3d::FileSystem::Extension(path).lowercased();
        if (extension != U"sus") {
            return Result<Chart>::makeError(U"Unsupported chart format: " + extension, path);
        }

        s3d::BinaryReader reader{ path };
        if (!reader) {
            return Result<Chart>::makeError(U"Failed to read the chart file.", path);
        }
        const s3d::Blob source{ reader };
        if (source.size() != static_cast<std::size_t>(reader.size())) {
            return Result<Chart>::makeError(U"Failed to read the complete chart file.", path);
        }
        const std::unique_ptr<CcResult, decltype(&cc_result_free)> parsed{
            cc_parse(CcFormat_Sus, CcMode_Xlair, reinterpret_cast<const uint8_t*>(source.data()), source.size()),
            cc_result_free,
        };
        if (const auto error = cc_result_error(parsed.get())) {
            return Result<Chart>::makeError(s3d::Unicode::FromUTF8(error), path);
        }
        auto result = compilation::Compile(cc_result_chart(parsed.get()), options);
        for (auto& diagnostic : result.diagnostics) {
            diagnostic.path = path;
        }
        return result;
    }
}
