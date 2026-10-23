#include "Result.hpp"

#include "core/user/Level.hpp"
#include "ui/Design.hpp"
#include "ui/assets/Assets.hpp"
#include "ui/audio/SoundEffect.hpp"
#include "ui/components/GameMusicPlate.hpp"
#include "ui/components/UserNameplate.hpp"
#include "ui/input/SliderInput.hpp"
#include "ui/localization/Localization.hpp"
#include "ui/presentation/ScoringText.hpp"
#include "ui/theme/Palette.hpp"

#include <array>
#include <utility>

namespace xlair::ui::scenes {
    namespace {
        constexpr double InputWaitSeconds = 2.0;
    }

    Result::Result(const InitData& init) : SceneBase{ init }, m_result{ std::move(getData().game_result) } {
        getData().game_result.reset();
        m_slider_mappings = {
            {
                .region = { .start = 0, .width = 16 },
                .label = localization::GetText(localization::TextId::ResultContinue),
                .color = theme::Palette::Cyan,
            },
        };
    }

    void Result::update() {
        m_elapsed += Scene::DeltaTime();
        if (m_elapsed < InputWaitSeconds) {
            return;
        }
        const auto* controller = getData().application->controller();
        if (KeyEnter.down() || input::TouchRegionDown(controller, 0, 16)) {
            audio::PlaySoundEffect(audio::SoundEffect::Confirm);
            getData().returning_from_game = true;
            changeScene(SceneState::MusicSelect, 500, CrossFade::No);
        }
    }

    void Result::draw() const {
        Scene::Rect().draw(theme::Palette::White);
        FontAsset{ assets::font::Display }(U"RESULT").drawAt(48, Vec2{ DesignSize.x / 2.0, 260 }, theme::Palette::Gray);

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
            components::DrawUserNameplate(
                {
                    .display_name = user->display_name,
                    .rating = user->rating,
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
                .max_plays = session.maxPlays(),
                .remaining_plays = session.remainingPlays(),
            },
            Point{ 1480, 72 },
            m_elapsed
        );
        const auto& score = result.score;
        m_score_bar.draw(
            { .score = score.score, .clear_gauge = score.clear_gauge },
            Point{ DesignSize.x / 2 - 434, 56 }
        );

        const auto display = FontAsset{ assets::font::Display };
        const auto text = FontAsset{ assets::font::Text };
        text(U"SCORE").draw(28, Vec2{ 400, 335 }, theme::Palette::Gray);
        display(U"{:07}"_fmt(score.score)).draw(96, Vec2{ 390, 365 }, theme::Palette::Cyan);
        display(presentation::GradeLabel(core::scoring::GradeForScore(score.score)))
            .drawAt(96, Vec2{ 1280, 410 }, theme::Palette::Cyan);
        text(presentation::ClearStatusLabel(core::scoring::ClearTypeForScore(score, result.total_combo)))
            .drawAt(32, Vec2{ 1280, 500 }, theme::Palette::Gray);

        const std::array counts{
            std::pair{ StringView{ U"PERFECT" }, score.perfect_count },
            std::pair{ StringView{ U"GREAT" }, score.great_count },
            std::pair{ StringView{ U"GOOD" }, score.good_count },
            std::pair{ StringView{ U"MISS" }, score.miss_count },
        };
        for (std::size_t index = 0; index < counts.size(); ++index) {
            const double y = 570.0 + index * 72.0;
            const auto& [label, count] = counts[index];
            text(label).draw(30, Vec2{ 400, y }, theme::Palette::Gray);
            display(count).draw(36, Arg::topRight(840, y), theme::Palette::Gray);
        }
        text(U"MAX COMBO").draw(28, Vec2{ 1100, 590 }, theme::Palette::Gray);
        display(U"{} / {}"_fmt(score.max_combo, result.total_combo)).draw(40, Vec2{ 1100, 635 }, theme::Palette::Pink);
        text(U"CLEAR GAUGE").draw(28, Vec2{ 1100, 730 }, theme::Palette::Gray);
        display(U"{:.1f}%"_fmt(score.clear_gauge * 100.0)).draw(40, Vec2{ 1100, 775 }, theme::Palette::Cyan);
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
