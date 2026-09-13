#pragma once

#include "Common.hpp"
#include "app/interfaces/IConfigLoader.hpp"

#include <memory>

namespace xlair::app {
    class Application {
    public:
        Application(FilePath data_directory, std::unique_ptr<interfaces::IConfigLoader> config_loader);

        [[nodiscard]]
        const FilePath& dataDirectory() const noexcept;

        [[nodiscard]]
        bool loadConfig();

        [[nodiscard]]
        const Optional<Config>& config() const noexcept;

        [[nodiscard]]
        const Optional<interfaces::ConfigLoadError>& configLoadError() const noexcept;

    private:
        FilePath m_data_directory;
        std::unique_ptr<interfaces::IConfigLoader> m_config_loader;
        Optional<Config> m_config;
        Optional<interfaces::ConfigLoadError> m_config_load_error;
    };
}
