#include "app/Application.hpp"

#include <utility>

namespace xlair::app {
    Application::Application(std::unique_ptr<interfaces::IConfigLoader> config_loader, ApiClientFactory api_factory)
        : m_config_loader{ std::move(config_loader) }, m_api_factory{ std::move(api_factory) } {}

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

    bool Application::loadConfig() {
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
