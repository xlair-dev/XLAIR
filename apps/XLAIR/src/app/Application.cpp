#include "Application.hpp"

#include <utility>

namespace xlair::app {
    const Optional<Config>& Application::config() const noexcept {
        return m_config;
    }

    api::IClient* Application::apiClient() noexcept {
        return m_api_client.get();
    }

    interfaces::ICardReader* Application::cardReader() noexcept {
        return m_card_reader.get();
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

    void Application::setCardReader(std::unique_ptr<interfaces::ICardReader> reader) {
        m_card_reader = std::move(reader);
    }

    void Application::setMusicCatalog(Array<sheets::Metadata> catalog) {
        m_music_catalog = std::move(catalog);
    }
}
