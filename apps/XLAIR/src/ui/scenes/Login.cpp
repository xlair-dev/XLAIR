#include "Login.hpp"

namespace xlair::ui::scenes {
    namespace {
        constexpr auto CancelButton = app::controller::MaintenanceButton::Button2;
    }

    Login::Login(const InitData& init) : SceneBase{ init } {
        ClearPrint();

        const auto& data = getData();
        if (!data.scanned_card) {
            Print << U"Card information is not available.";
            return;
        }

        auto* api_client = data.application->apiClient();
        if (!api_client) {
            Print << U"The API client is not available.";
            return;
        }

        m_login_flow.start(*api_client, data.scanned_card->card_id);
        reportState();
    }

    void Login::update() {
        auto& data = getData();
        const auto* controller = data.application->controller();
        if (controller && controller->maintenanceButton(CancelButton).down()) {
            m_login_flow.cancel();
            ClearPrint();
            changeScene(SceneState::Title, 0);
            return;
        }

        const auto previous = m_login_flow.state();
        m_login_flow.update(data.application->creditPool());
        if (m_login_flow.state() != previous) {
            reportState();
        }
    }

    void Login::draw() const {}

    void Login::reportState() const {
        switch (m_login_flow.state()) {
            case app::flows::Login::State::Idle:
                break;

            case app::flows::Login::State::FindingUser:
                Print << U"Card ID: " + getData().scanned_card->card_id;
                Print << U"Looking up the user...";
                Print << U"Press F3 to cancel.";
                break;

            case app::flows::Login::State::WaitingForCredit:
                Print << U"Insert a coin (F4).";
                break;

            case app::flows::Login::State::IncrementingCredits:
                Print << U"Updating Credits...";
                break;

            case app::flows::Login::State::UserFound: {
                const auto& user = *m_login_flow.user();
                Print << U"User found.";
                Print << U"ID: " + user.id;
                Print << U"Display name: " + user.display_name;
                Print << U"Rating: {}"_fmt(user.rating);
                Print << U"XP: {}"_fmt(user.xp);
                Print << U"Credits: {}"_fmt(user.credits);
                break;
            }

            case app::flows::Login::State::RegistrationRequired:
                Print << U"User registration is required.";
                break;

            case app::flows::Login::State::Failed: {
                const auto& error = *m_login_flow.error();
                Print << U"Login failed.";
                Print << error.message;
                if (error.status_code) {
                    Print << U"HTTP {}"_fmt(*error.status_code);
                }
                break;
            }
        }
    }
}
