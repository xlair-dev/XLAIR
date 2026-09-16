#include "Application.hpp"

#include <utility>

namespace xlair::app {
    const Optional<Config>& Application::config() const noexcept {
        return m_config;
    }

    api::IClient* Application::apiClient() noexcept {
        return m_api_client.get();
    }

    const Array<api::Music>& Application::catalog() const noexcept {
        return m_catalog;
    }

    void Application::setConfig(Config config) {
        m_config = std::move(config);
    }

    void Application::setApiClient(std::unique_ptr<api::IClient> client) {
        m_api_client = std::move(client);
    }

    void Application::setCatalog(Array<api::Music> catalog) {
        m_catalog = std::move(catalog);
    }
}
