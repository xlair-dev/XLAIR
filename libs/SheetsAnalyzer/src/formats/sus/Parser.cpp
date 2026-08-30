#include "Parser.hpp"

namespace xlair::sheets::formats::sus {
    Result<Document> Parse(const s3d::FilePath& path) {
        Result<Document> result;
        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::Error,
            .message = U"The new SUS parser has not been migrated yet.",
            .path = path,
            .line = s3d::none,
            .column = s3d::none,
        });
        return result;
    }
}
