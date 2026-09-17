#include "Application.hpp"

#include <utility>

namespace xlair::app {
    const Optional<Config>& Application::config() const noexcept {
        return m_config;
    }

    api::IClient* Application::apiClient() noexcept {
        return m_api_client.get();
    }

    const Array<sheets::Metadata>& Application::musicCatalog() const noexcept {
        return m_music_catalog;
    }

    void Application::setConfig(Config config) {
        m_config = std::move(config);
    }

    void Application::setApiClient(std::unique_ptr<api::IClient> client) {
        m_api_client = std::move(client);
    }

    void Application::setMusicCatalog(Array<sheets::Metadata> catalog) {
        m_music_catalog = std::move(catalog);
    }
}
