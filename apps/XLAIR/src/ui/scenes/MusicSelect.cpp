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
#include "ui/localization/Localization.hpp"
#include "ui/presentation/DifficultyText.hpp"
#include "ui/presentation/ScoringText.hpp"
#include "ui/primitives/Arrow.hpp"
#include "ui/theme/DifficultyTheme.hpp"
#include "ui/theme/Palette.hpp"

namespace xlair::ui::scenes {
    namespace {
        constexpr SizeF SelectedCardSize{ 416, 545 };
        constexpr SizeF SideCardSize = SelectedCardSize * 0.88;
        constexpr double CardY = 553.0;
        constexpr double SelectedCardMargin = 50.0;
        constexpr double CardSpacing = 50.0;

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

        [[nodiscard]]
        bool TouchRegionDown(const app::controller::Controller* controller, const uint32 start, const uint32 width) {
            if (!controller) {
                return false;
            }

            const std::size_t first_zone = static_cast<std::size_t>(start) * 2;
            const std::size_t last_zone =
                Min(static_cast<std::size_t>(start + width) * 2, app::controller::TouchZoneCount);
            for (std::size_t zone = first_zone; zone < last_zone; ++zone) {
                if (controller->touchZone(zone).down()) {
                    return true;
                }
            }
            return false;
        }

    }

    MusicSelect::MusicSelect(const InitData& init) : SceneBase{ init }, m_slider_mappings{ MakeSliderMappings() } {
        const auto& application = *getData().application;
        m_flow = std::make_unique<app::flows::MusicSelect>(application.musicCatalog());
        if (application.config()) {
            m_remaining_seconds = application.config()->system.menu_timer_seconds;
        }
    }

    void MusicSelect::update() {
        handleInput();
        updateAnimation();
        if (m_flow) {
            if (const auto* music = m_flow->selectedMusic()) {
                m_preview_player.update(music->music, music->demo_start_seconds);
            } else {
                m_preview_player.stop();
            }
        }
        m_text_elapsed += Scene::DeltaTime();
        m_remaining_seconds = Max(0.0, m_remaining_seconds - Scene::DeltaTime());
    }

    void MusicSelect::draw() const {
        Scene::Rect().draw(theme::Palette::White);

        if (!m_flow || m_flow->empty()) {
            drawEmptyCatalog();
        } else {
            drawCards();
            drawArrows();
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
                .remaining_seconds = static_cast<int32>(Ceil(m_remaining_seconds)),
                .max_plays = max_plays,
                .remaining_plays = remaining_plays,
            },
            Point{ 1599, 72 }
        );
    }

    void MusicSelect::handleInput() {
        if (!m_flow || m_flow->empty()) {
            return;
        }

        const auto* controller = getData().application->controller();
        const bool move_left = KeyLeft.down() || TouchRegionDown(controller, 0, 3);
        const bool move_right = KeyRight.down() || TouchRegionDown(controller, 3, 3);
        const bool select = KeyEnter.down() || TouchRegionDown(controller, 6, 4);
        const bool difficulty_down = KeyDown.down() || TouchRegionDown(controller, 10, 2);
        const bool difficulty_up = KeyUp.down() || TouchRegionDown(controller, 12, 2);

        if (move_left && m_flow->moveMusic(-1)) {
            audio::PlaySoundEffect(audio::SoundEffect::Navigate);
            m_scroll_offset = 1.0;
            m_text_elapsed = 0.0;
        } else if (move_right && m_flow->moveMusic(1)) {
            audio::PlaySoundEffect(audio::SoundEffect::Navigate);
            m_scroll_offset = -1.0;
            m_text_elapsed = 0.0;
        }

        if (difficulty_down && m_flow->moveDifficulty(-1)) {
            audio::PlaySoundEffect(audio::SoundEffect::ChangeDifficulty);
            m_text_elapsed = 0.0;
        } else if (difficulty_up && m_flow->moveDifficulty(1)) {
            audio::PlaySoundEffect(audio::SoundEffect::ChangeDifficulty);
            m_text_elapsed = 0.0;
        }

        if (select) {
            const auto* music = m_flow->selectedMusic();
            const auto* difficulty = m_flow->selectedDifficulty();
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

    void MusicSelect::updateAnimation() {
        m_scroll_offset = Math::SmoothDamp(m_scroll_offset, 0.0, m_scroll_velocity, 0.1);
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
        const std::size_t selected_index = m_flow->selectedIndex();
        const double scroll = m_scroll_offset;
        const double scroll_abs = Abs(scroll);
        constexpr Vec2 Center{ DesignSize.x / 2.0, CardY };
        constexpr double NeighborGap =
            SelectedCardSize.x / 2.0 + SideCardSize.x / 2.0 + CardSpacing + SelectedCardMargin;

        const SizeF selected_size = SelectedCardSize.lerp(SideCardSize, scroll_abs);
        const double selected_x = Center.x - NeighborGap * scroll;
        drawCard(selected_index, RectF{ Arg::center = Vec2{ selected_x, CardY }, selected_size }, m_text_elapsed);

        const auto draw_side = [&](const int32 direction) {
            const double directional_scroll = direction * scroll;
            const double margin_factor = Min(1.0, 1.0 + directional_scroll);
            const double neighbor_scale = Clamp(directional_scroll, 0.0, 1.0);
            double x =
                selected_x + direction * (selected_size.x / 2.0 + CardSpacing + SelectedCardMargin * margin_factor);

            for (int64 index = static_cast<int64>(selected_index) + direction;
                 index >= 0 && index < static_cast<int64>(m_flow->musicCount());
                 index += direction) {
                if (x - direction * SideCardSize.x > DesignSize.x || x - direction * SideCardSize.x < 0) {
                    break;
                }

                SizeF card_size = SideCardSize;
                if (index == static_cast<int64>(selected_index) + direction) {
                    card_size = SideCardSize.lerp(SelectedCardSize, neighbor_scale);
                    x += direction * (CardSpacing + SelectedCardMargin) * neighbor_scale;
                }

                const RectF region{
                    Arg::center = Vec2{ x + direction * card_size.x / 2.0, CardY },
                    card_size,
                };
                drawCard(static_cast<std::size_t>(index), region, 0.0);
                x += direction * (CardSpacing + SideCardSize.x);
            }
        };

        draw_side(1);
        draw_side(-1);
    }

    void MusicSelect::drawCard(const std::size_t music_index, const RectF& region, const double text_elapsed) const {
        const auto* music = m_flow->musicAt(music_index);
        if (!music) {
            return;
        }
        const auto* difficulty = m_flow->difficultyFor(*music);
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

    void MusicSelect::drawArrows() const {
        constexpr Vec2 Center{ DesignSize.x / 2.0, CardY };
        constexpr Vec2 Right = Center.movedBy(SelectedCardSize.x / 2.0 - 10, 0);
        constexpr Vec2 Left = Center.movedBy(-SelectedCardSize.x / 2.0 + 10, 0);
        primitives::DrawArrow(Right, primitives::ArrowDirection::Right, theme::Palette::Gray);
        primitives::DrawArrow(Right.movedBy(30, 0), primitives::ArrowDirection::Right, theme::Palette::Gray);
        primitives::DrawArrow(Left, primitives::ArrowDirection::Left, theme::Palette::Gray);
        primitives::DrawArrow(Left.movedBy(-30, 0), primitives::ArrowDirection::Left, theme::Palette::Gray);
    }

    void MusicSelect::drawEmptyCatalog() const {
        FontAsset{ assets::font::Text }(localization::GetText(localization::TextId::MusicSelectEmptyCatalog))
            .drawAt(34, Vec2{ DesignSize.x / 2.0, CardY }, theme::Palette::Gray);
    }
}
