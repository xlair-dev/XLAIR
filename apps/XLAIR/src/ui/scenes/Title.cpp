#include "Title.hpp"

#include "app/Version.hpp"
#include "ui/Design.hpp"
#include "ui/assets/Assets.hpp"
#include "ui/theme/Palette.hpp"

namespace xlair::ui::scenes {
    namespace {
        constexpr auto PrimaryMaintenanceButton = app::controller::MaintenanceButton::Button1;
    }

    Title::Title(const InitData& init) : SceneBase{ init } {
        m_login_flow = std::make_unique<app::flows::Login>(*getData().application->cardReader());
        m_login_flow->start();
        reportState();
    }

    void Title::update() {
        const auto previous = m_login_flow->state();
        const auto* controller = getData().application->controller();
        const bool maintenance_button_down =
            controller && controller->maintenanceButton(PrimaryMaintenanceButton).down();
        if ((previous == app::flows::Login::State::CardRead || previous == app::flows::Login::State::Failed) &&
            maintenance_button_down) {
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
        Scene::Rect().draw(theme::Palette::White);

        const Vec2 center{ DesignSize.x / 2.0, DesignSize.y / 2.0 };
        TextureAsset{ assets::texture::Logo }.drawAt(center.x, DesignSize.y * 0.4);

        const auto prompt_region = FontAsset{ assets::font::Text }(U"カードをタッチして始める")
                                       .drawAt(32, Vec2{ center.x, 760 }, theme::Palette::Cyan)
                                       .stretched(30);
        RectF{ Arg::rightCenter = prompt_region.leftCenter(), 135, 2 }.draw(theme::Palette::Cyan);
        RectF{ Arg::leftCenter = prompt_region.rightCenter(), 135, 2 }.draw(theme::Palette::Cyan);

        FontAsset{ assets::font::Text }(U"XLAIR version: {}"_fmt(app::version::String))
            .draw(15, Arg::bottomRight(DesignSize.x - 10, DesignSize.y - 5), theme::Palette::Gray);
    }

    void Title::reportState() const {
        switch (m_login_flow->state()) {
            case app::flows::Login::State::Idle:
                break;

            case app::flows::Login::State::WaitingForCard:
                Logger << U"[Title] Waiting for a card...";
                if (getData().application->config()->card_reader.mode == app::Config::CardReader::Mode::Mock) {
                    Logger << U"[Title] Press Space to scan the mock card.";
                }
                break;

            case app::flows::Login::State::CardRead:
                Logger << U"[Title] Card ID: " + m_login_flow->card()->card_id;
                Logger << U"[Title] Press maintenance button 1 to scan again.";
                break;

            case app::flows::Login::State::Failed:
                Logger << U"[Title] Card reader error: " + m_login_flow->error()->message;
                Logger << U"[Title] Press maintenance button 1 to retry.";
                break;
        }
    }
}
