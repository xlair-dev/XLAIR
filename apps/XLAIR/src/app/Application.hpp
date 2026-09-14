#pragma once

#include "Common.hpp"
#include "app/interfaces/IConfigLoader.hpp"
#include <ApiClient/IClient.hpp>

#include <memory>
#include <functional>

namespace xlair::app {
    class Application {
    public:
        enum class CatalogSyncState { Idle, Loading, Succeeded, Failed };

        using ApiClientFactory = std::function<std::unique_ptr<api::IClient>(const Config::Api&)>;
        using LocalSyncFactory = std::function<api::Request<bool>(api::IClient&, const Array<api::Music>&, URLView)>;

        Application(
            std::unique_ptr<interfaces::IConfigLoader> config_loader,
            ApiClientFactory api_factory,
            LocalSyncFactory sync_factory
        );

        [[nodiscard]]
        bool initializeApi();

        [[nodiscard]]
        api::IClient* apiClient() noexcept;

        void startCatalogSync();

        void updateCatalogSync();

        [[nodiscard]]
        CatalogSyncState catalogSyncState() const noexcept;

        [[nodiscard]]
        const Array<api::Music>& catalog() const noexcept;

        [[nodiscard]]
        const Optional<api::ApiError>& catalogSyncError() const noexcept;

        [[nodiscard]]
        bool loadConfig();

        [[nodiscard]]
        const Optional<Config>& config() const noexcept;

        [[nodiscard]]
        const Optional<interfaces::ConfigLoadError>& configLoadError() const noexcept;

    private:
        Optional<Config> m_config;
        std::unique_ptr<interfaces::IConfigLoader> m_config_loader;
        Optional<interfaces::ConfigLoadError> m_config_load_error;
        ApiClientFactory m_api_factory;
        std::unique_ptr<api::IClient> m_api_client;
        LocalSyncFactory m_sync_factory;
        api::Request<bool> m_local_sync;
        Array<api::Music> m_pending_catalog;
        api::Request<Array<api::Music>> m_catalog_request;
        CatalogSyncState m_catalog_sync_state = CatalogSyncState::Idle;
        Array<api::Music> m_catalog;
        Optional<api::ApiError> m_catalog_sync_error;
    };
}
