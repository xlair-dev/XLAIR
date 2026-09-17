#pragma once

#include "Common.hpp"
#include "app/config/Config.hpp"
#include "app/interfaces/ICardReader.hpp"

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
        interfaces::ICardReader* cardReader() noexcept;

        [[nodiscard]]
        const Array<sheets::Metadata>& musicCatalog() const noexcept;

    private:
        friend class BootFlow;

        void setConfig(Config config);
        void setApiClient(std::unique_ptr<api::IClient> client);
        void setCardReader(std::unique_ptr<interfaces::ICardReader> reader);
        void setMusicCatalog(Array<sheets::Metadata> catalog);

        Optional<Config> m_config;
        std::unique_ptr<api::IClient> m_api_client;
        std::unique_ptr<interfaces::ICardReader> m_card_reader;
        Array<sheets::Metadata> m_music_catalog;
    };
}
