#include "Application.hpp"

#include <cmath>
#include <utility>

namespace xlair::app {
    namespace {
        constexpr auto CoinButton = controller::MaintenanceButton::Button3;
    }

    void Application::update(const bool accept_credit_input) {
        if (!m_controller || m_controller_error) {
            m_credit_input.suspend();
            return;
        }

        auto result = m_controller->update();
        if (!result) {
            m_credit_input.suspend();
            if (result.error) {
                m_controller_error = std::move(*result.error);
            }
            return;
        }

        const auto& coin = m_controller->maintenanceButton(CoinButton);
        if (!coin.available()) {
            m_credit_input.suspend();
            return;
        }
        // Observe disabled input too, without carrying a held press out of Maintenance.
        const bool credit_press = m_credit_input.update(coin.pressed(), Time::GetMillisec());
        if (accept_credit_input && coin.down() && credit_press) {
            m_credit_pool.insert();
        }
    }

    const Optional<Config>& Application::config() const noexcept {
        return m_config;
    }

    bool Application::hasUnsavedConfig() const noexcept {
        return m_config && m_saved_config && *m_config != *m_saved_config;
    }

    bool Application::setApiTimeoutSeconds(const double seconds) {
        if (!m_config || !m_api_client || !m_api_client->setTimeoutSeconds(seconds)) {
            return false;
        }
        m_config->api.timeout_seconds = seconds;
        return true;
    }

    bool Application::setAudioVolumes(const Config::Audio& volumes) noexcept {
        if (!m_config) {
            return false;
        }
        for (const double volume : { volumes.music_volume, volumes.ui_sound_volume, volumes.gameplay_sound_volume }) {
            if (!std::isfinite(volume) || volume < 0.0 || volume > 1.0) {
                return false;
            }
        }
        m_config->audio = volumes;
        return true;
    }

    void Application::setTouchThreshold(const uint8 threshold) noexcept {
        if (!m_config || !m_controller) {
            return;
        }
        m_controller->setTouchThreshold(threshold);
        const auto applied = m_controller->touchThreshold();
        if (!applied) {
            return;
        }
        switch (m_config->controller.mode) {
            case Config::Controller::Mode::GroundSlider:
                m_config->controller.ground_slider.touch_threshold = *applied;
                break;
            case Config::Controller::Mode::V1:
                m_config->controller.v1.touch_threshold = *applied;
                break;
            case Config::Controller::Mode::Keyboard:
                break;
        }
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

    bool Application::autoplay() const noexcept {
        return m_config && m_config->system.autoplay;
    }

    void Application::setAutoplay(const bool enabled) noexcept {
        if (m_config) {
            m_config->system.autoplay = enabled;
        }
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
        m_saved_config = m_config;
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
