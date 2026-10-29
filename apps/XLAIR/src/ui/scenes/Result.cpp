#include "Result.hpp"

#include "core/user/Level.hpp"
#include "ui/Design.hpp"
#include "ui/assets/Assets.hpp"
#include "ui/audio/AudioBus.hpp"
#include "ui/audio/SoundEffect.hpp"
#include "ui/components/GameMusicPlate.hpp"
#include "ui/components/ResultPanel.hpp"
#include "ui/components/UserNameplate.hpp"
#include "ui/localization/Localization.hpp"
#include "ui/presentation/ResultRating.hpp"
#include "ui/theme/Palette.hpp"

#include <utility>

namespace xlair::ui::scenes {
    namespace {
        constexpr double InputWaitSeconds = 2.0;
        constexpr double AutoContinueSeconds = 20.0;
        constexpr Duration MusicFadeOut{ 0.25 };
    }

    Result::Result(const InitData& init)
        : SceneBase{ init }, m_result{ std::move(getData().game_result) }, m_auto_continue{ AutoContinueSeconds } {
        getData().game_result.reset();
        m_slider_mappings = {
            {
                .region = { .start = 0, .width = 16 },
                .label = localization::GetText(localization::TextId::ResultContinue),
                .color = theme::Palette::Pink,
            },
        };
        AudioAsset{ assets::bgm::Result }.play(audio::Music);
    }

    Result::~Result() {
        // FinalResult takes over the same playing asset without restarting it.
        if (!m_continue_music) {
            AudioAsset{ assets::bgm::Result }.stop();
        }
    }

    void Result::update() {
        if (m_leaving) {
            return;
        }
        m_elapsed += Scene::DeltaTime();
        const bool timed_out = m_auto_continue.update(true, Scene::DeltaTime());
        const auto* controller = getData().application->controller();
        constexpr std::array MenuKeys{ KeyEnter };
        m_menu_input.update(controller, MenuKeys, Scene::DeltaTime(), m_elapsed >= InputWaitSeconds);
        if (m_elapsed < InputWaitSeconds) {
            return;
        }
        if (timed_out || m_menu_input.keyDown(KeyEnter) || m_menu_input.touchDown(0, 16)) {
            continueToNext();
        }
    }

    void Result::continueToNext() {
        auto& data = getData();
        const bool finished = data.application->playSession().finished();
        if (!changeScene(finished ? SceneState::FinalResult : SceneState::MusicSelect, 500, CrossFade::No)) {
            return;
        }
        m_leaving = true;
        m_continue_music = finished;
        if (!finished) {
            const auto& config = data.application->config();
            data.ensureMusicSelectContext().resetTimer(config ? config->system.menu_timer_seconds : 0.0);
            AudioAsset{ assets::bgm::Result }.stop(MusicFadeOut);
            data.returning_from_game = true;
        }
        audio::PlaySoundEffect(audio::SoundEffect::Finish);
    }

    void Result::draw() const {
        Scene::Rect().draw(theme::Palette::White);
        TextureAsset{ assets::texture::ResultBackground }.resized(DesignSize).draw();

        if (m_result) {
            drawResult(*m_result);
        } else {
            FontAsset{ assets::font::Text }(localization::GetText(localization::TextId::ResultUnavailable))
                .drawAt(28, Vec2{ DesignSize.x / 2.0, DesignSize.y / 2.0 }, theme::Palette::Gray);
        }
        if (m_elapsed >= InputWaitSeconds) {
            components::DrawSliderMappingGuide(m_slider_mappings, RectF{ 210, 1010, 1500, 70 });
        }
    }

    void Result::drawResult(const GameResult& result) const {
        const auto& session = getData().application->playSession();
        if (const auto* user = session.user()) {
            const auto level = core::user::CalculateLevelProgress(user->xp);
            const auto rating = presentation::ResultRatingAt(result.previous_rating, result.rating, m_elapsed);
            components::DrawUserNameplate(
                {
                    .display_name = user->display_name,
                    .rating = rating.value_or(user->rating),
                    .level = level.level,
                    .level_progress = level.progress,
                },
                Point{ 59, 72 }
            );
        }
        components::DrawGameMusicPlate(
            {
                .title = result.selection.title,
                .jacket = getData().jackets.get(result.selection.music_id),
                .difficulty_index = result.selection.difficulty.index,
                .level = result.selection.difficulty.level,
                .max_plays = result.selection.max_plays,
                .remaining_plays = result.selection.remaining_plays,
            },
            Point{ 1480, 72 },
            m_elapsed
        );
        const auto& score = result.score;
        m_score_bar.draw(
            { .score = score.score, .clear_gauge = score.clear_gauge },
            Point{ DesignSize.x / 2 - 434, 56 }
        );

        components::DrawResultPanel(
            {
                .score = score,
                .total_combo = result.total_combo,
                .previous_rating = result.previous_rating,
                .rating = result.rating,
            },
            Point{ DesignSize.x / 2 - 700, 200 },
            m_elapsed
        );
    }

    void Result::updateFadeIn(const double) {
        m_elapsed += Scene::DeltaTime();
    }

    void Result::updateFadeOut(const double) {
        m_elapsed += Scene::DeltaTime();
    }

    void Result::drawFadeIn(const double t) const {
        drawFadeOverlay(1.0 - Clamp(t, 0.0, 1.0));
    }

    void Result::drawFadeOut(const double t) const {
        drawFadeOverlay(Clamp(t, 0.0, 1.0));
    }

    void Result::drawFadeOverlay(const double opacity) const {
        draw();
        Scene::Rect().draw(ColorF{ 0.0, 0.0, 0.0, opacity });
    }
}
