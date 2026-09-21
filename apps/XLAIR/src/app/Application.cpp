#include "Application.hpp"

#include <utility>

namespace xlair::app {
    namespace {
        constexpr auto CoinButton = controller::MaintenanceButton::Button3;
    }

    void Application::update() {
        if (!m_controller || m_controller_error) {
            return;
        }

        auto result = m_controller->update();
        if (!result) {
            if (result.error) {
                m_controller_error = std::move(*result.error);
            }
            return;
        }

        if (m_controller->maintenanceButton(CoinButton).down()) {
            m_credit_pool.insert();
        }
    }

    const Optional<Config>& Application::config() const noexcept {
        return m_config;
    }

    api::IClient* Application::apiClient() noexcept {
        return m_api_client.get();
    }

    interfaces::ICardReader* Application::cardReader() noexcept {
        return m_card_reader.get();
    }

    app::controller::Controller* Application::controller() noexcept {
        return m_controller.get();
    }

    const Optional<app::controller::Error>& Application::controllerError() const noexcept {
        return m_controller_error;
    }

    credits::CreditPool& Application::creditPool() noexcept {
        return m_credit_pool;
    }

    const credits::CreditPool& Application::creditPool() const noexcept {
        return m_credit_pool;
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

    void Application::setController(std::unique_ptr<app::controller::Controller> controller) {
        m_controller = std::move(controller);
        m_controller_error.reset();
    }

    void Application::setMusicCatalog(Array<sheets::Metadata> catalog) {
        m_music_catalog = std::move(catalog);
    }
}
