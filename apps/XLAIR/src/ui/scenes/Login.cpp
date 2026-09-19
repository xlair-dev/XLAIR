#include "Login.hpp"

namespace xlair::ui::scenes {
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
        const auto previous = m_login_flow.state();
        m_login_flow.update();
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

            case app::flows::Login::State::UserNotFound:
                Print << U"The card is not registered to a user.";
                break;

            case app::flows::Login::State::Failed: {
                const auto& error = *m_login_flow.error();
                Print << U"Failed to look up the user.";
                Print << error.message;
                if (error.status_code) {
                    Print << U"HTTP {}"_fmt(*error.status_code);
                }
                break;
            }
        }
    }
}
