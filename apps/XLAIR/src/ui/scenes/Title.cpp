#include "Title.hpp"

#include "app/Version.hpp"
#include "ui/Design.hpp"
#include "ui/assets/Assets.hpp"
#include "ui/localization/Localization.hpp"
#include "ui/theme/Palette.hpp"
#include "ui/transitions/ScreenWipe.hpp"

namespace xlair::ui::scenes {
    Title::Title(const InitData& init) : SceneBase{ init } {
        getData().title_to_login_wipe = false;
        getData().music_select_context.reset();
        getData().scanned_card.reset();
        getData().game_result.reset();
        getData().application->playSession().reset();
        m_title_flow = std::make_unique<app::flows::Title>(*getData().application->cardReader());
        m_title_flow->start();
        reportState();
    }

    void Title::update() {
        m_elapsed += Scene::DeltaTime();
#ifndef NDEBUG
        if (KeyP.down() || KeyF11.down()) {
            changeScene(SceneState::PlayfieldDebug, 0);
            return;
        }
        if (KeyG.down()) {
            changeScene(SceneState::ComponentGallery, 0);
            return;
        }
#endif
        const auto previous = m_title_flow->state();
        m_title_flow->update();
        if (m_title_flow->state() != previous) {
            reportState();
            if (m_title_flow->state() == app::flows::Title::State::CardRead) {
                getData().scanned_card = *m_title_flow->card();
                getData().title_to_login_wipe = true;
                if (!changeScene(SceneState::Login, transitions::ScreenWipeMillisec, CrossFade::No)) {
                    getData().title_to_login_wipe = false;
                }
            }
        }
    }

    void Title::draw() const {
        m_background.draw(m_elapsed);

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
#ifndef NDEBUG
        FontAsset{ assets::font::Label }(U"P: 3D PLAYFIELD DEBUG")
            .draw(16, Vec2{ 24, DesignSize.y - 54 }, theme::Palette::Gray);
        FontAsset{ assets::font::Label }(U"G: COMPONENT GALLERY")
            .draw(16, Vec2{ 24, DesignSize.y - 28 }, theme::Palette::Gray);
#endif
    }

    void Title::updateFadeOut(const double) {
        m_elapsed += Scene::DeltaTime();
    }

    void Title::drawFadeOut(const double t) const {
        if (getData().title_to_login_wipe) {
            draw();
            transitions::DrawScreenWipe(Clamp(t, 0.0, 1.0) * 0.5);
        } else {
            SceneBase::drawFadeOut(t);
        }
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
                break;

            case app::flows::Title::State::Failed:
                Logger << U"[Title] Card reader error: " + m_title_flow->error()->message;
                break;
        }
    }
}
