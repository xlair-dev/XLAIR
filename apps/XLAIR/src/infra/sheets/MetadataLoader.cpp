#include "MetadataLoader.hpp"

#include <utility>

namespace xlair::infra::sheets {
    MetadataLoader::MetadataLoader(s3d::FilePath directory) : m_directory{ std::move(directory) } {}

    xlair::sheets::Result<s3d::Array<xlair::sheets::Metadata>> MetadataLoader::load() const {
        if (!s3d::FileSystem::Exists(m_directory) && !s3d::FileSystem::CreateDirectories(m_directory)) {
            return xlair::sheets::Result<s3d::Array<xlair::sheets::Metadata>>::makeError(
                U"Failed to create the sheet metadata directory.",
                m_directory
            );
        }

        return xlair::sheets::ScanMetadata(m_directory);
    }
}
