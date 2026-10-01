#pragma once

#include "app/interfaces/IMetadataLoader.hpp"

namespace xlair::infra::sheets {
    class MetadataLoader final : public app::interfaces::IMetadataLoader {
    public:
        MetadataLoader(s3d::FilePath local_directory, s3d::FilePath sync_directory);

        [[nodiscard]]
        xlair::sheets::Result<s3d::Array<xlair::sheets::Metadata>> load() const override;

    private:
        s3d::FilePath m_local_directory;
        s3d::FilePath m_sync_directory;
    };
}
