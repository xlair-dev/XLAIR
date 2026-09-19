#include "Title.hpp"

namespace xlair::ui::scenes {
    Title::Title(const InitData& init) : SceneBase{ init } {
        m_login_flow = std::make_unique<app::flows::Login>(*getData().application);
        m_login_flow->start();
        reportState();
    }

    void Title::update() {
        const auto previous = m_login_flow->state();
        if ((previous == app::flows::Login::State::CardRead || previous == app::flows::Login::State::Failed) &&
            KeyR.down()) {
            ClearPrint();
            m_login_flow->start();
            reportState();
            return;
        }

        m_login_flow->update();
        if (m_login_flow->state() != previous) {
            reportState();
        }
    }

    void Title::draw() const {
        m_font(U"XLAIR").draw(40, 40, Palette::White);
    }

    void Title::reportState() const {
        switch (m_login_flow->state()) {
            case app::flows::Login::State::Idle:
                break;

            case app::flows::Login::State::WaitingForCard:
                Print << U"Waiting for a card...";
                if (getData().application->config()->card_reader.mode == app::Config::CardReader::Mode::Mock) {
                    Print << U"Press Space to scan the mock card.";
                }
                break;

            case app::flows::Login::State::CardRead:
                Print << U"Card ID: " + m_login_flow->card()->card_id;
                Print << U"Press R to scan again.";
                break;

            case app::flows::Login::State::Failed:
                Print << U"Card reader error: " + m_login_flow->error()->message;
                Print << U"Press R to retry.";
                break;
        }
    }
}
