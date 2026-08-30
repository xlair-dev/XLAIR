#include "Loader.hpp"

namespace xlair::sheets::metadata {
    Result<Metadata> Load(const s3d::FilePath& path) {
        return Result<Metadata>::makeError(U"The metadata loader has not been migrated yet.", path);
    }

    Result<s3d::Array<Metadata>> Scan(const s3d::FilePath& directory) {
        return Result<s3d::Array<Metadata>>::makeError(U"The metadata scanner has not been migrated yet.", directory);
    }
}
