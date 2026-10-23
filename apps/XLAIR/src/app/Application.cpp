#include "Application.hpp"

#include <cmath>
#include <utility>

namespace xlair::app {
    namespace {
        constexpr auto CoinButton = controller::MaintenanceButton::Button3;
    }

    void Application::update(const bool accept_credit_input) {
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

        if (accept_credit_input && m_controller->maintenanceButton(CoinButton).down()) {
            m_credit_pool.insert();
        }
    }

    const Optional<Config>& Application::config() const noexcept {
        return m_config;
    }

    bool Application::setApiTimeoutSeconds(const double seconds) {
        if (!m_config || !m_api_client || !m_api_client->setTimeoutSeconds(seconds)) {
            return false;
        }
        m_config->api.timeout_seconds = seconds;
        return true;
    }

    void Application::setTouchThresholdHighlight(const bool enabled) noexcept {
        if (m_config) {
            m_config->controller.highlight_touch_threshold = enabled;
        }
    }

    void Application::setInputLatencyOffsetSeconds(const double seconds) noexcept {
        if (m_config && std::isfinite(seconds)) {
            m_config->input.latency_offset_seconds = seconds;
        }
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

    session::PlaySession& Application::playSession() noexcept {
        return m_play_session;
    }

    const session::PlaySession& Application::playSession() const noexcept {
        return m_play_session;
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
