#include "SheetsAnalyzer.hpp"

#include "chart/Loader.hpp"
#include "metadata/Loader.hpp"

namespace xlair::sheets {
    Result<Metadata> LoadMetadata(const s3d::FilePath& path) {
        return metadata::Load(path);
    }

    Result<s3d::Array<Metadata>>
    ScanMetadata(const s3d::FilePath& directory, const s3d::Array<s3d::FilePath>& excluded_directories) {
        return metadata::Scan(directory, excluded_directories);
    }

    Result<Chart> LoadChart(const s3d::FilePath& path, const ChartLoadOptions& options) {
        return chart::Load(path, options);
    }
}
