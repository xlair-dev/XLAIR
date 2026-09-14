#include "app/Application.hpp"

#include <utility>

namespace xlair::app {
    Application::Application(
        std::unique_ptr<interfaces::IConfigLoader> config_loader,
        ApiClientFactory api_factory,
        LocalSyncFactory sync_factory
    )
        : m_config_loader{ std::move(config_loader) }, m_api_factory{ std::move(api_factory) },
          m_sync_factory{ std::move(sync_factory) } {}

    bool Application::initializeApi() {
        if (!m_config || !m_api_factory) {
            return false;
        }
        m_api_client = m_api_factory(m_config->api);
        return static_cast<bool>(m_api_client);
    }

    api::IClient* Application::apiClient() noexcept {
        return m_api_client.get();
    }

    void Application::startCatalogSync() {
        if (m_local_sync) {
            m_local_sync->cancel();
        }
        m_local_sync.reset();
        m_pending_catalog.clear();
        if (m_catalog_request) {
            m_catalog_request->cancel();
            m_catalog_request.reset();
        }
        m_catalog_sync_error.reset();
        if (!m_api_client || !m_sync_factory) {
            m_catalog_sync_error = api::ApiError{
                api::ErrorKind::Configuration,
                U"The API client is not initialized.",
                none,
            };
            m_catalog_sync_state = CatalogSyncState::Failed;
            return;
        }
        m_catalog_request = m_api_client->fetchCatalog();
        if (!m_catalog_request) {
            m_catalog_sync_error = api::ApiError{
                api::ErrorKind::Configuration,
                U"The API client did not create a catalog request.",
                none,
            };
            m_catalog_sync_state = CatalogSyncState::Failed;
            return;
        }
        m_catalog_sync_state = CatalogSyncState::Loading;
    }

    void Application::updateCatalogSync() {
        if (m_catalog_sync_state != CatalogSyncState::Loading) {
            return;
        }
        if (m_local_sync) {
            m_local_sync->update();
            const auto& local = m_local_sync->result();
            if (!local) {
                return;
            }
            if (const auto* error = std::get_if<api::ApiError>(&*local)) {
                m_catalog_sync_error = *error;
                m_catalog_sync_state = CatalogSyncState::Failed;
            } else {
                m_catalog = std::move(m_pending_catalog);
                m_catalog_sync_state = CatalogSyncState::Succeeded;
            }
            m_local_sync.reset();
            return;
        }
        m_catalog_request->update();
        const auto& result = m_catalog_request->result();
        if (!result) {
            return;
        }
        if (const auto* error = std::get_if<api::ApiError>(&*result)) {
            m_catalog_sync_error = *error;
            m_catalog_sync_state = CatalogSyncState::Failed;
        } else {
            m_pending_catalog = std::get<Array<api::Music>>(*result);
            m_local_sync = m_sync_factory(*m_api_client, m_pending_catalog, m_config->api.endpoint);
            if (!m_local_sync) {
                m_catalog_sync_error = api::ApiError{
                    api::ErrorKind::Configuration,
                    U"Could not create local catalog sync.",
                    none,
                };
                m_catalog_sync_state = CatalogSyncState::Failed;
            }
        }
        m_catalog_request.reset();
    }

    Application::CatalogSyncState Application::catalogSyncState() const noexcept {
        return m_catalog_sync_state;
    }

    const Array<api::Music>& Application::catalog() const noexcept {
        return m_catalog;
    }

    const Optional<api::ApiError>& Application::catalogSyncError() const noexcept {
        return m_catalog_sync_error;
    }

    bool Application::loadConfig() {
        if (m_local_sync) {
            m_local_sync->cancel();
        }
        m_local_sync.reset();
        m_pending_catalog.clear();
        if (m_catalog_request) {
            m_catalog_request->cancel();
        }
        m_catalog_request.reset();
        m_catalog_sync_state = CatalogSyncState::Idle;
        m_catalog_sync_error.reset();
        m_catalog.clear();
        m_api_client.reset();
        m_config.reset();
        m_config_load_error.reset();

        if (!m_config_loader) {
            m_config_load_error = interfaces::ConfigLoadError{
                .message = U"A config loader is not available.",
                .path = U"",
            };
            return false;
        }

        auto result = m_config_loader->load();
        if (!result) {
            m_config_load_error = std::move(result.error);
            return false;
        }

        m_config = std::move(result.value);
        return true;
    }

    const Optional<Config>& Application::config() const noexcept {
        return m_config;
    }

    const Optional<interfaces::ConfigLoadError>& Application::configLoadError() const noexcept {
        return m_config_load_error;
    }
}
