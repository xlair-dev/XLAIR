#include "MusicSelect.hpp"

#include "core/scoring/Grade.hpp"
#include "core/user/Level.hpp"
#include "ui/Design.hpp"
#include "ui/assets/Assets.hpp"
#include "ui/audio/SoundEffect.hpp"
#include "ui/components/MenuHeader.hpp"
#include "ui/components/MenuTimerPlate.hpp"
#include "ui/components/SliderMappingGuide.hpp"
#include "ui/components/UserNameplate.hpp"
#include "ui/input/SliderInput.hpp"
#include "ui/localization/Localization.hpp"
#include "ui/presentation/DifficultyText.hpp"
#include "ui/presentation/ScoringText.hpp"
#include "ui/theme/DifficultyTheme.hpp"
#include "ui/theme/Palette.hpp"

namespace xlair::ui::scenes {
    namespace {
        constexpr double CardY = 553.0;

        [[nodiscard]]
        Array<components::SliderMapping> MakeSliderMappings() {
            return {
                {
                    .region = { .start = 0, .width = 3, .right_corner = false },
                    .label = U"◀ {}"_fmt(localization::GetText(localization::TextId::MusicSelectSliderMoveLeft)),
                    .color = theme::Palette::Pink,
                },
                {
                    .region = { .start = 3, .width = 3, .left_corner = false },
                    .label = U"{} ▶"_fmt(localization::GetText(localization::TextId::MusicSelectSliderMoveRight)),
                    .color = theme::Palette::Pink,
                },
                {
                    .region = { .start = 6, .width = 4 },
                    .label = localization::GetText(localization::TextId::MusicSelectSliderConfirm),
                    .color = theme::Palette::Cyan,
                },
                {
                    .region = { .start = 10, .width = 2, .right_corner = false },
                    .label = localization::GetText(localization::TextId::MusicSelectSliderDifficultyDown),
                    .color = theme::Palette::Pink,
                },
                {
                    .region = { .start = 12, .width = 2, .left_corner = false },
                    .label = localization::GetText(localization::TextId::MusicSelectSliderDifficultyUp),
                    .color = theme::Palette::Pink,
                },
                {
                    .region = { .start = 14, .width = 2 },
                    .label = localization::GetText(localization::TextId::MusicSelectSliderSettings),
                    .color = theme::Palette::Purple,
                },
            };
        }

    }

    MusicSelect::MusicSelect(const InitData& init)
        : SceneBase{ init },
          m_card_carousel{ components::MusicCard::size(), Vec2{ DesignSize.x / 2.0, CardY }, DesignSize.x },
          m_slider_mappings{ MakeSliderMappings() } {
        getData().ensureMusicSelectContext();
    }

    void MusicSelect::update() {
        handleInput();
        m_card_carousel.update(Scene::DeltaTime());
        m_text_elapsed += Scene::DeltaTime();
    }

    void MusicSelect::draw() const {
        Scene::Rect().draw(theme::Palette::White);

        if (getData().music_select_context->flow().empty()) {
            drawEmptyCatalog();
        } else {
            drawCards();
            const auto& flow = getData().music_select_context->flow();
            m_card_carousel.drawArrows(flow.selectedIndex(), flow.musicCount(), theme::Palette::Gray);
        }

        components::DrawMenuHeader(Assets::Header);
        components::DrawSliderMappingGuide(m_slider_mappings, RectF{ 210, 1010, 1500, 70 });

        const auto& config = getData().application->config();
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

        const uint32 max_plays =
            session.active() ? session.maxPlays() : (config ? static_cast<uint32>(Max(0, config->system.playable)) : 0);
        const uint32 remaining_plays = session.active() ? session.remainingPlays() : max_plays;
        components::DrawMenuTimerPlate(
            {
                .remaining_seconds = static_cast<int32>(Ceil(getData().music_select_context->remainingSeconds())),
                .max_plays = max_plays,
                .remaining_plays = remaining_plays,
            },
            Point{ 1599, 72 }
        );
    }

    void MusicSelect::handleInput() {
        const auto* controller = getData().application->controller();
        if (KeyTab.down() || input::TouchRegionDown(controller, 14, 2)) {
            audio::PlaySoundEffect(audio::SoundEffect::Navigate);
            changeScene(SceneState::Settings, 0);
            return;
        }

        auto& flow = getData().music_select_context->flow();
        if (flow.empty()) {
            return;
        }

        const bool move_left = KeyLeft.down() || input::TouchRegionDown(controller, 0, 3);
        const bool move_right = KeyRight.down() || input::TouchRegionDown(controller, 3, 3);
        const bool select = KeyEnter.down() || input::TouchRegionDown(controller, 6, 4);
        const bool difficulty_down = KeyDown.down() || input::TouchRegionDown(controller, 10, 2);
        const bool difficulty_up = KeyUp.down() || input::TouchRegionDown(controller, 12, 2);

        if (move_left && flow.moveMusic(-1)) {
            audio::PlaySoundEffect(audio::SoundEffect::Navigate);
            m_card_carousel.animateMove(-1);
            m_text_elapsed = 0.0;
        } else if (move_right && flow.moveMusic(1)) {
            audio::PlaySoundEffect(audio::SoundEffect::Navigate);
            m_card_carousel.animateMove(1);
            m_text_elapsed = 0.0;
        }

        if (difficulty_down && flow.moveDifficulty(-1)) {
            audio::PlaySoundEffect(audio::SoundEffect::ChangeDifficulty);
            m_text_elapsed = 0.0;
        } else if (difficulty_up && flow.moveDifficulty(1)) {
            audio::PlaySoundEffect(audio::SoundEffect::ChangeDifficulty);
            m_text_elapsed = 0.0;
        }

        if (select) {
            const auto* music = flow.selectedMusic();
            const auto* difficulty = flow.selectedDifficulty();
            if (music && difficulty) {
                if (difficulty->src.isEmpty()) {
                    Logger << U"[MusicSelect] Sheet '{}' is not implemented."_fmt(difficulty->id);
                } else {
                    audio::PlaySoundEffect(audio::SoundEffect::Confirm);
                    Logger << U"[MusicSelect] Selected music '{}' / sheet '{}'."_fmt(music->id, difficulty->id);
                }
            }
        }
    }

    void MusicSelect::RegisterAssets() {
        if (!TextureAsset::Register(
                Assets::Header,
                components::MakeMenuHeaderTexture(
                    U"MUSIC SELECT",
                    localization::GetText(localization::TextId::MusicSelectPrompt)
                )
            ) ||
            !TextureAsset::Load(Assets::Header)) {
            throw Error{ U"Failed to register the MusicSelect header texture." };
        }
    }

    void MusicSelect::drawCards() const {
        const auto& flow = getData().music_select_context->flow();
        for (const auto& placement : m_card_carousel.layout(flow.selectedIndex(), flow.musicCount())) {
            drawCard(placement.index, placement.region, placement.selected ? m_text_elapsed : 0.0);
        }
    }

    void MusicSelect::drawCard(const std::size_t music_index, const RectF& region, const double text_elapsed) const {
        const auto& flow = getData().music_select_context->flow();
        const auto* music = flow.musicAt(music_index);
        if (!music) {
            return;
        }
        const auto* difficulty = flow.difficultyFor(*music);
        if (!difficulty) {
            return;
        }

        const auto* record = getData().application->playSession().record(difficulty->id);
        const uint32 high_score = record ? record->score : 0;
        const String difficulty_label = presentation::DifficultyLabel(difficulty->index);
        const StringView grade =
            record ? presentation::GradeLabel(core::scoring::GradeForScore(high_score)) : StringView{};
        const StringView clear_status = record ? presentation::ClearStatusLabel(record->clear_type) : StringView{};
        const components::MusicCardData data{
            .title = music->title,
            .artist = music->artist,
            .difficulty = difficulty_label,
            .level = difficulty->level,
            .designer = difficulty->designer,
            .high_score = high_score,
            .grade = grade,
            .clear_status = clear_status,
            .available = !difficulty->src.isEmpty(),
        };
        region.drawShadow(Vec2{ 12, 26 }, 32, 0, ColorF{ 0, 0, 0, 0.22 });
        region(m_music_card.render(
                   data,
                   getData().jackets.get(music->id),
                   theme::GetDifficultyTheme(difficulty->index),
                   text_elapsed
               ))
            .draw();
    }

    void MusicSelect::drawEmptyCatalog() const {
        FontAsset{ assets::font::Text }(localization::GetText(localization::TextId::MusicSelectEmptyCatalog))
            .drawAt(34, Vec2{ DesignSize.x / 2.0, CardY }, theme::Palette::Gray);
    }
}
