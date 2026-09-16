#include "Factories.hpp"

#include "ClientOptions.hpp"
#include "LocalCatalogSync.hpp"

#include <ApiClient/HttpClient.hpp>
#include <ApiClient/MockClient.hpp>

#include <utility>

namespace xlair::infra::api {
    std::unique_ptr<xlair::api::IClient> CreateClient(const app::Config::Api& config) {
        if (config.mode == app::Config::Api::Mode::Mock) {
            return std::make_unique<xlair::api::MockClient>(xlair::api::MockClientOptions{
                .data_directory = config.mock.data_directory,
            });
        }
        return std::make_unique<xlair::api::HttpClient>(ToClientOptions(config));
    }

    LocalCatalogSyncFactory::LocalCatalogSyncFactory(s3d::FilePath sheets_directory)
        : m_sheets_directory{ std::move(sheets_directory) } {}

    xlair::api::Request<bool> LocalCatalogSyncFactory::operator()(
        xlair::api::IClient& client,
        const s3d::Array<xlair::api::Music>& catalog,
        const s3d::URLView endpoint
    ) const {
        return SyncCatalog(client, catalog, m_sheets_directory, endpoint);
    }
}
