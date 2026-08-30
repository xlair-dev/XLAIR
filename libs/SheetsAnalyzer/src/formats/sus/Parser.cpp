#include "Parser.hpp"

namespace xlair::sheets::formats::sus {
    Result<Document> Parse(const s3d::FilePath& path) {
        return Result<Document>::makeError(U"The new SUS parser has not been migrated yet.", path);
    }
}
