#pragma once

#include "app/config/Config.hpp"

#include <ApiClient/IClient.hpp>

#include <memory>

namespace xlair::infra::api {
    [[nodiscard]]
    std::unique_ptr<xlair::api::IClient> CreateClient(const app::Config::Api& config);

    class LocalCatalogSyncFactory {
    public:
        explicit LocalCatalogSyncFactory(s3d::FilePath sheets_directory);

        [[nodiscard]]
        xlair::api::Request<bool> operator()(
            xlair::api::IClient& client,
            const s3d::Array<xlair::api::Music>& catalog,
            s3d::URLView endpoint
        ) const;

    private:
        s3d::FilePath m_sheets_directory;
    };
}
