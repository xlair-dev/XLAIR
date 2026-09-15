#pragma once

#include "Common.hpp"
#include "app/config/Config.hpp"

#include <ApiClient/IClient.hpp>

#include <memory>

namespace xlair::app {
    class Application {
    public:
        [[nodiscard]]
        const Optional<Config>& config() const noexcept;

        [[nodiscard]]
        api::IClient* apiClient() noexcept;

        [[nodiscard]]
        const Array<api::Music>& catalog() const noexcept;

    private:
        friend class BootFlow;

        void setConfig(Config config);
        void setApiClient(std::unique_ptr<api::IClient> client);
        void setCatalog(Array<api::Music> catalog);

        Optional<Config> m_config;
        std::unique_ptr<api::IClient> m_api_client;
        Array<api::Music> m_catalog;
    };
}
