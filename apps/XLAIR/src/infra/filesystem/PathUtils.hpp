#pragma once

#include <Siv3D/FileSystem.hpp>

#include <filesystem>

namespace xlair::infra::filesystem {
    [[nodiscard]]
    inline std::filesystem::path NormalizedPath(const s3d::FilePathView path) {
        auto full_path = s3d::FileSystem::FullPath(path);
        auto normalized = std::filesystem::path{ full_path.toUTF32() }.lexically_normal();
        std::error_code error;
        const auto canonical = std::filesystem::weakly_canonical(normalized, error);
        if (!error) {
            normalized = canonical;
        }
        if (!normalized.has_filename()) {
            normalized = normalized.parent_path();
        }
#if defined(_WIN32)
        normalized = std::filesystem::path{ s3d::String{ normalized.u32string() }.lowercased().toUTF32() };
#endif
        return normalized;
    }

    [[nodiscard]]
    inline bool IsSameOrWithin(const s3d::FilePathView path, const s3d::FilePathView directory) {
        const auto child = NormalizedPath(path);
        const auto parent = NormalizedPath(directory);
        auto child_part = child.begin();
        for (auto parent_part = parent.begin(); parent_part != parent.end(); ++parent_part, ++child_part) {
            if (child_part == child.end() || *child_part != *parent_part) {
                return false;
            }
        }
        return true;
    }
}
