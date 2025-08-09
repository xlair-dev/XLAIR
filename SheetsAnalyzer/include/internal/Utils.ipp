#pragma once
#define NO_S3D_USING
#include <Siv3D.hpp>

namespace SheetsAnalyzer::internal {
    inline s3d::FilePath HandlePath(s3d::FilePathView metadata_path, s3d::FilePathView path) {
        if (not path.empty()) {
            const auto current_path = s3d::FileSystem::CurrentDirectory();
            const auto parent_path = s3d::FileSystem::ParentPath(metadata_path);

            if (path.starts_with(U"/")) {
                return s3d::FileSystem::PathAppend(current_path, path.substr(1));
            } else {
                return s3d::FileSystem::PathAppend(parent_path, path);
            }
        }
        return U"";
    }
}
