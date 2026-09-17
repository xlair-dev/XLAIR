#pragma once

#include "Common.hpp"
#include "app/config/Config.hpp"

#include <ApiClient/IClient.hpp>
#include <SheetsAnalyzer/Metadata.hpp>

#include <memory>

namespace xlair::app {
    class Application {
    public:
        [[nodiscard]]
        const Optional<Config>& config() const noexcept;

        [[nodiscard]]
        api::IClient* apiClient() noexcept;

        [[nodiscard]]
        const Array<sheets::Metadata>& musicCatalog() const noexcept;

    private:
        friend class BootFlow;

        void setConfig(Config config);
        void setApiClient(std::unique_ptr<api::IClient> client);
        void setMusicCatalog(Array<sheets::Metadata> catalog);

        Optional<Config> m_config;
        std::unique_ptr<api::IClient> m_api_client;
        Array<sheets::Metadata> m_music_catalog;
    };
}
