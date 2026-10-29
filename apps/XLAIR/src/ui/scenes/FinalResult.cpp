#include "FinalResult.hpp"

#include "ui/Design.hpp"
#include "ui/assets/Assets.hpp"
#include "ui/audio/AudioBus.hpp"
#include "ui/audio/SoundEffect.hpp"
#include "ui/components/ScrollingText.hpp"
#include "ui/localization/Localization.hpp"
#include "ui/theme/Palette.hpp"

namespace xlair::ui::scenes {
    namespace {
        constexpr double InputWaitSeconds = 2.0;
        constexpr double AutoReturnSeconds = 10.0;
    }

    FinalResult::FinalResult(const InitData& init) : SceneBase{ init } {
        if (getData().music_select_context) {
            getData().music_select_context->pause();
        }
        m_save_flow.start(getData().application->apiClient(), getData().application->playSession());
        // Normally Result's loop is already playing. Also support direct scene entry.
        const AudioAsset music{ assets::bgm::Result };
        if (!music.isPlaying()) {
            music.play(audio::Music);
        }
    }

    FinalResult::~FinalResult() {
        AudioAsset{ assets::bgm::Result }.stop();
    }

    void FinalResult::update() {
        m_elapsed += Scene::DeltaTime();
        updateSave();
        const auto* controller = getData().application->controller();
        constexpr std::array MenuKeys{ KeyEnter };
        m_menu_input.update(controller, MenuKeys, Scene::DeltaTime(), m_elapsed >= InputWaitSeconds);
        const bool confirm = m_menu_input.keyDown(KeyEnter) || m_menu_input.touchDown(0, 16);
        m_return_requested |= confirm || m_elapsed >= AutoReturnSeconds;

        // Keep the return request while saving; Title resets the session history.
        if (m_save_flow.state() == app::flows::FinalResult::State::Saved && m_return_requested) {
            AudioAsset{ assets::bgm::Result }.stop(Duration{ 0.25 });
            changeScene(SceneState::Title, 500, CrossFade::No);
            return;
        }
        if (confirm && m_save_flow.canRetry()) {
            audio::PlaySoundEffect(audio::SoundEffect::Confirm);
            m_save_flow.retry();
        }
    }

    void FinalResult::updateFadeIn(const double) {
        m_elapsed += Scene::DeltaTime();
        updateSave();
    }

    void FinalResult::updateSave() {
        const auto previous = m_save_flow.state();
        m_save_flow.update();
        if (m_save_flow.state() == app::flows::FinalResult::State::Failed && m_save_flow.state() != previous) {
            Logger << U"[FinalResult] Save failed: " + m_save_flow.error()->message;
        }
    }

    void FinalResult::draw() const {
        Scene::Rect().draw(theme::Palette::White);
        FontAsset{ assets::font::Text }(localization::GetText(localization::TextId::FinalResultThankYou))
            .drawAt(48, Vec2{ DesignSize.x / 2.0, DesignSize.y / 2.0 }, theme::Palette::Gray);

        if (const auto& error = m_save_flow.error()) {
            const auto status = m_save_flow.canRetry() ? localization::TextId::FinalResultSaveFailed
                                                       : localization::TextId::FinalResultSaveUnconfirmed;
            FontAsset{ assets::font::Text }(localization::GetText(status))
                .drawAt(24, Vec2{ DesignSize.x / 2.0, 900 }, theme::Palette::Gray);
            components::DrawScrollingText(
                FontAsset{ assets::font::Text },
                error->message,
                20,
                Rect{ 210, 940, 1500, 30 },
                m_elapsed,
                theme::Palette::Gray
            );
            if (m_elapsed >= InputWaitSeconds && m_save_flow.canRetry()) {
                FontAsset{ assets::font::Text }(localization::GetText(localization::TextId::FinalResultRetry))
                    .drawAt(24, Vec2{ DesignSize.x / 2.0, 1000 }, theme::Palette::Gray);
            }
        }
    }

    void FinalResult::drawFadeIn(const double t) const {
        draw();
        Scene::Rect().draw(ColorF{ 0.0, 0.0, 0.0, 1.0 - Clamp(t, 0.0, 1.0) });
    }

    void FinalResult::drawFadeOut(const double t) const {
        draw();
        Scene::Rect().draw(ColorF{ 0.0, 0.0, 0.0, Clamp(t, 0.0, 1.0) });
    }
}
