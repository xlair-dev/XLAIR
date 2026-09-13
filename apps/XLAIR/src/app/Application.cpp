#include "app/Application.hpp"

#include <utility>

namespace xlair::app {
    Application::Application(FilePath data_directory, std::unique_ptr<interfaces::IConfigLoader> config_loader)
        : m_data_directory{ std::move(data_directory) }, m_config_loader{ std::move(config_loader) } {}

    const FilePath& Application::dataDirectory() const noexcept {
        return m_data_directory;
    }

    bool Application::loadConfig() {
        m_config.reset();
        m_config_load_error.reset();

        if (!m_config_loader) {
            m_config_load_error = interfaces::ConfigLoadError{
                .message = U"A config loader is not available.",
                .path = U"",
            };
            return false;
        }

        const auto result = m_config_loader->load();
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
