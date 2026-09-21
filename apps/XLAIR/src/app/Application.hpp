#pragma once

#include "Common.hpp"
#include "app/controller/Controller.hpp"
#include "app/config/Config.hpp"
#include "app/credits/CreditPool.hpp"
#include "app/interfaces/ICardReader.hpp"

#include <ApiClient/IClient.hpp>
#include <SheetsAnalyzer/Metadata.hpp>

#include <memory>

namespace xlair::app::flows {
    class Boot;
}

namespace xlair::app {
    class Application {
    public:
        void update();

        [[nodiscard]]
        const Optional<Config>& config() const noexcept;

        [[nodiscard]]
        api::IClient* apiClient() noexcept;

        [[nodiscard]]
        interfaces::ICardReader* cardReader() noexcept;

        [[nodiscard]]
        app::controller::Controller* controller() noexcept;

        [[nodiscard]]
        const Optional<app::controller::Error>& controllerError() const noexcept;

        [[nodiscard]]
        credits::CreditPool& creditPool() noexcept;

        [[nodiscard]]
        const credits::CreditPool& creditPool() const noexcept;

        [[nodiscard]]
        const Array<sheets::Metadata>& musicCatalog() const noexcept;

    private:
        friend class flows::Boot;

        void setConfig(Config config);
        void setApiClient(std::unique_ptr<api::IClient> client);
        void setCardReader(std::unique_ptr<interfaces::ICardReader> reader);
        void setController(std::unique_ptr<app::controller::Controller> controller);
        void setMusicCatalog(Array<sheets::Metadata> catalog);

        Optional<Config> m_config;
        std::unique_ptr<api::IClient> m_api_client;
        std::unique_ptr<interfaces::ICardReader> m_card_reader;
        std::unique_ptr<app::controller::Controller> m_controller;
        Optional<app::controller::Error> m_controller_error;
        credits::CreditPool m_credit_pool;
        Array<sheets::Metadata> m_music_catalog;
    };
}
