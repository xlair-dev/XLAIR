#pragma once

#include "app/interfaces/IMetadataLoader.hpp"

namespace xlair::infra::sheets {
    class MetadataLoader final : public app::interfaces::IMetadataLoader {
    public:
        explicit MetadataLoader(s3d::FilePath directory);

        [[nodiscard]]
        xlair::sheets::Result<s3d::Array<xlair::sheets::Metadata>> load() const override;

    private:
        s3d::FilePath m_directory;
    };
}
