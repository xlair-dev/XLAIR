#include "SheetsAnalyzer.hpp"
#include "internal/AnalyzeFile.hpp"

namespace SheetsAnalyzer {
    s3d::Array<Metadata> AnalyzeAll(const s3d::FilePath& directory) {
        s3d::Array<Metadata> metadataList;
        for (const auto& path : s3d::FileSystem::DirectoryContents(directory, s3d::Recursive::Yes)) {
            if (s3d::FileSystem::IsDirectory(path)) {
                continue; // Skip directories
            }
            if (s3d::FileSystem::FileName(path).starts_with(U"music")) {
                if (const auto metadata = Analyze(path)) {
                    metadataList.push_back(*metadata);
                }
            }
        }
        return metadataList;
    }

    s3d::Optional<Metadata> Analyze(const s3d::FilePath& path) {
        const auto extension = s3d::FileSystem::Extension(path);
        if (extension == U"json") {
            return internal::AnalyzeJsonFile(path);
        }
        else {
            return s3d::none; // Unsupported file type
        }
    }
}
