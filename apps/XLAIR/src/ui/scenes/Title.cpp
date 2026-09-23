#include "Title.hpp"

#include "app/Version.hpp"
#include "ui/Design.hpp"
#include "ui/assets/Assets.hpp"
#include "ui/localization/Localization.hpp"
#include "ui/theme/Palette.hpp"

namespace xlair::ui::scenes {
    namespace {
        constexpr auto PrimaryMaintenanceButton = app::controller::MaintenanceButton::Button1;
    }

    Title::Title(const InitData& init) : SceneBase{ init } {
        getData().scanned_card.reset();
        getData().application->playSession().reset();
        m_title_flow = std::make_unique<app::flows::Title>(*getData().application->cardReader());
        m_title_flow->start();
        reportState();
    }

    void Title::update() {
        const auto previous = m_title_flow->state();
        const auto* controller = getData().application->controller();
        const bool maintenance_button_down =
            controller && controller->maintenanceButton(PrimaryMaintenanceButton).down();
        if ((previous == app::flows::Title::State::CardRead || previous == app::flows::Title::State::Failed) &&
            maintenance_button_down) {
            m_title_flow->start();
            reportState();
            return;
        }

        m_title_flow->update();
        if (m_title_flow->state() != previous) {
            reportState();
            if (m_title_flow->state() == app::flows::Title::State::CardRead) {
                getData().scanned_card = *m_title_flow->card();
                changeScene(SceneState::Login, 0);
            }
        }
    }

    void Title::draw() const {
        Scene::Rect().draw(theme::Palette::White);

        const Vec2 center{ DesignSize.x / 2.0, DesignSize.y / 2.0 };
        TextureAsset{ assets::texture::Logo }.drawAt(center.x, DesignSize.y * 0.4);

        const auto prompt_region =
            FontAsset{ assets::font::Text }(localization::GetText(localization::TextId::TitleTouchToStart))
                .drawAt(32, Vec2{ center.x, 760 }, theme::Palette::Cyan)
                .stretched(30);
        RectF{ Arg::rightCenter = prompt_region.leftCenter(), 135, 2 }.draw(theme::Palette::Cyan);
        RectF{ Arg::leftCenter = prompt_region.rightCenter(), 135, 2 }.draw(theme::Palette::Cyan);

        FontAsset{ assets::font::Text }(U"XLAIR version: {}"_fmt(app::version::String))
            .draw(15, Arg::bottomRight(DesignSize.x - 10, DesignSize.y - 5), theme::Palette::Gray);
    }

    void Title::reportState() const {
        switch (m_title_flow->state()) {
            case app::flows::Title::State::Idle:
                break;

            case app::flows::Title::State::WaitingForCard:
                Logger << U"[Title] Waiting for a card...";
                if (getData().application->config()->card_reader.mode == app::Config::CardReader::Mode::Mock) {
                    Logger << U"[Title] Press Space to scan the mock card.";
                }
                break;

            case app::flows::Title::State::CardRead:
                Logger << U"[Title] Card ID: " + m_title_flow->card()->card_id;
                Logger << U"[Title] Press maintenance button 1 to scan again.";
                break;

            case app::flows::Title::State::Failed:
                Logger << U"[Title] Card reader error: " + m_title_flow->error()->message;
                Logger << U"[Title] Press maintenance button 1 to retry.";
                break;
        }
    }
}
