#pragma once

#include "Common.hpp"
#include "app/interfaces/IConfigLoader.hpp"

#include <memory>

namespace xlair::app {
    class Application {
    public:
        Application(std::unique_ptr<interfaces::IConfigLoader> config_loader);

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
    };
}
