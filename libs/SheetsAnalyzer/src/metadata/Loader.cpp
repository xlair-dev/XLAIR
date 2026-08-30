#include "Loader.hpp"

namespace xlair::sheets::metadata {
    Result<Metadata> Load(const s3d::FilePath& path) {
        Result<Metadata> result;
        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::Error,
            .message = U"The metadata loader has not been migrated yet.",
            .path = path,
            .line = s3d::none,
            .column = s3d::none,
        });
        return result;
    }

    Result<s3d::Array<Metadata>> Scan(const s3d::FilePath& directory) {
        Result<s3d::Array<Metadata>> result;
        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::Error,
            .message = U"The metadata scanner has not been migrated yet.",
            .path = directory,
            .line = s3d::none,
            .column = s3d::none,
        });
        return result;
    }
}
