#pragma once

#include "Common.hpp"
#include "app/interfaces/IConfigLoader.hpp"
#include <ApiClient/IClient.hpp>

#include <memory>
#include <functional>

namespace xlair::app {
    class Application {
    public:
        using ApiClientFactory = std::function<std::unique_ptr<api::IClient>(const Config::Api&)>;

        Application(std::unique_ptr<interfaces::IConfigLoader> config_loader, ApiClientFactory api_factory);

        [[nodiscard]]
        bool initializeApi();

        [[nodiscard]]
        api::IClient* apiClient() noexcept;

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
    };
}
