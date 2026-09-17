#include "MetadataLoader.hpp"

#include <utility>

namespace xlair::infra::sheets {
    MetadataLoader::MetadataLoader(s3d::FilePath directory) : m_directory{ std::move(directory) } {}

    xlair::sheets::Result<s3d::Array<xlair::sheets::Metadata>> MetadataLoader::load() const {
        return xlair::sheets::ScanMetadata(m_directory);
    }
}
