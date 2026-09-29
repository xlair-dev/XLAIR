#include "Settings.hpp"

#include "core/user/Level.hpp"
#include "ui/Design.hpp"
#include "ui/audio/SoundEffect.hpp"
#include "ui/components/MenuHeader.hpp"
#include "ui/components/MenuTimerPlate.hpp"
#include "ui/components/UserNameplate.hpp"
#include "ui/input/SliderInput.hpp"
#include "ui/localization/Localization.hpp"
#include "ui/primitives/Arrow.hpp"
#include "ui/theme/Palette.hpp"

namespace xlair::ui::scenes {
    namespace {
        constexpr SizeF SelectedCardSize{ components::SettingCard::size() };
        constexpr SizeF SideCardSize = SelectedCardSize * 0.88;
        constexpr double CardY = 553.0;
        constexpr double SelectedCardMargin = 50.0;
        constexpr double CardSpacing = 50.0;

        // Provisional bounds for the UI prototype, not the final game-option constraints.
        constexpr int32 MinimumSpeedSteps = 1;
        constexpr int32 MaximumSpeedSteps = 100;
        constexpr int32 MinimumJudgmentOffsetMs = -100;
        constexpr int32 MaximumJudgmentOffsetMs = 100;

        [[nodiscard]]
        Array<components::SettingCardData> MakeSettingCards() {
            // Placeholder values for layout review; not connected to player options yet.
            return {
                {
                    .title = U"SPEED",
                    .description = U"Note scroll speed",
                    .value = U"1.0",
                    .unit = U"×",
                },
                {
                    .title = U"JUDGMENT OFFSET",
                    .description = U"Adjust judgment timing",
                    .value = U"+0",
                    .unit = U"ms",
                },
                {
                    .title = U"MIRROR",
                    .description = U"Reverse lane order",
                    .value = U"OFF",
                    .unit = U"",
                },
            };
        }

        [[nodiscard]]
        Array<components::SliderMapping> MakeSliderMappings() {
            return {
                {
                    .region = { .start = 0, .width = 3, .right_corner = false },
                    .label = U"◀ {}"_fmt(localization::GetText(localization::TextId::SettingsSliderPreviousItem)),
                    .color = theme::Palette::Pink,
                },
                {
                    .region = { .start = 3, .width = 3, .left_corner = false },
                    .label = U"{} ▶"_fmt(localization::GetText(localization::TextId::SettingsSliderNextItem)),
                    .color = theme::Palette::Pink,
                },
                {
                    .region = { .start = 6, .width = 4 },
                    .label = localization::GetText(localization::TextId::SettingsSliderConfirm),
                    .color = theme::Palette::Cyan,
                },
                {
                    .region = { .start = 10, .width = 2, .right_corner = false },
                    .label = localization::GetText(localization::TextId::SettingsSliderDecrease),
                    .color = theme::Palette::Pink,
                },
                {
                    .region = { .start = 12, .width = 2, .left_corner = false },
                    .label = localization::GetText(localization::TextId::SettingsSliderIncrease),
                    .color = theme::Palette::Pink,
                },
                {
                    .region = { .start = 14, .width = 2 },
                    .label = localization::GetText(localization::TextId::SettingsSliderBack),
                    .color = theme::Palette::Purple,
                },
            };
        }
    }

    Settings::Settings(const InitData& init)
        : SceneBase{ init }, m_setting_cards{ MakeSettingCards() }, m_slider_mappings{ MakeSliderMappings() } {
        getData().ensureMusicSelectContext();
    }

    void Settings::update() {
        handleInput();
        m_scroll_offset = Math::SmoothDamp(m_scroll_offset, 0.0, m_scroll_velocity, 0.1);
    }

    void Settings::handleInput() {
        const auto* controller = getData().application->controller();
        if (KeyTab.down() || input::TouchRegionDown(controller, 14, 2)) {
            audio::PlaySoundEffect(audio::SoundEffect::Navigate);
            changeScene(SceneState::MusicSelect, 0);
            return;
        }

        const bool move_left = KeyLeft.down() || input::TouchRegionDown(controller, 0, 3);
        const bool move_right = KeyRight.down() || input::TouchRegionDown(controller, 3, 3);
        const bool decrease = KeyDown.down() || input::TouchRegionDown(controller, 10, 2);
        const bool increase = KeyUp.down() || input::TouchRegionDown(controller, 12, 2);

        if (move_left && moveItem(-1)) {
            audio::PlaySoundEffect(audio::SoundEffect::Navigate);
        } else if (move_right && moveItem(1)) {
            audio::PlaySoundEffect(audio::SoundEffect::Navigate);
        }

        if (decrease && adjustValue(-1)) {
            audio::PlaySoundEffect(audio::SoundEffect::ChangeDifficulty);
        } else if (increase && adjustValue(1)) {
            audio::PlaySoundEffect(audio::SoundEffect::ChangeDifficulty);
        }
    }

    bool Settings::moveItem(const int32 direction) {
        if (m_setting_cards.isEmpty() || direction == 0) {
            return false;
        }

        const int64 destination = static_cast<int64>(m_selected_index) + (direction < 0 ? -1 : 1);
        if (destination < 0 || destination >= static_cast<int64>(m_setting_cards.size())) {
            return false;
        }

        m_selected_index = static_cast<std::size_t>(destination);
        m_scroll_offset = direction < 0 ? 1.0 : -1.0;
        return true;
    }

    bool Settings::adjustValue(const int32 direction) {
        if (m_setting_cards.isEmpty() || direction == 0) {
            return false;
        }

        auto& card = m_setting_cards[m_selected_index];
        const int32 step = direction < 0 ? -1 : 1;
        switch (static_cast<SettingItem>(m_selected_index)) {
            case SettingItem::Speed: {
                const int32 next = Clamp(m_note_speed_steps + step, MinimumSpeedSteps, MaximumSpeedSteps);
                if (next == m_note_speed_steps) {
                    return false;
                }
                m_note_speed_steps = next;
                card.value = U"{:.1f}"_fmt(m_note_speed_steps / 10.0);
                return true;
            }
            case SettingItem::JudgmentOffset: {
                const int32 next = Clamp(m_judgment_offset_ms + step, MinimumJudgmentOffsetMs, MaximumJudgmentOffsetMs);
                if (next == m_judgment_offset_ms) {
                    return false;
                }
                m_judgment_offset_ms = next;
                card.value = U"{:+}"_fmt(m_judgment_offset_ms);
                return true;
            }
            case SettingItem::Mirror: {
                const bool next = direction > 0;
                if (next == m_mirror) {
                    return false;
                }
                m_mirror = next;
                card.value = m_mirror ? U"ON" : U"OFF";
                return true;
            }
        }
        return false;
    }

    void Settings::draw() const {
        Scene::Rect().draw(theme::Palette::White);

        drawCards();
        drawArrows();

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

    void Settings::drawCards() const {
        if (m_setting_cards.isEmpty()) {
            return;
        }

        const double scroll = m_scroll_offset;
        const double scroll_abs = Abs(scroll);
        constexpr Vec2 Center{ DesignSize.x / 2.0, CardY };
        constexpr double NeighborGap =
            SelectedCardSize.x / 2.0 + SideCardSize.x / 2.0 + CardSpacing + SelectedCardMargin;

        const SizeF selected_size = SelectedCardSize.lerp(SideCardSize, scroll_abs);
        const double selected_x = Center.x - NeighborGap * scroll;
        drawCard(m_selected_index, RectF{ Arg::center = Vec2{ selected_x, CardY }, selected_size });

        const auto draw_side = [&](const int32 direction) {
            const double directional_scroll = direction * scroll;
            const double margin_factor = Min(1.0, 1.0 + directional_scroll);
            const double neighbor_scale = Clamp(directional_scroll, 0.0, 1.0);
            double x =
                selected_x + direction * (selected_size.x / 2.0 + CardSpacing + SelectedCardMargin * margin_factor);

            for (int64 index = static_cast<int64>(m_selected_index) + direction;
                 index >= 0 && index < static_cast<int64>(m_setting_cards.size());
                 index += direction) {
                if (x - direction * SideCardSize.x > DesignSize.x || x - direction * SideCardSize.x < 0) {
                    break;
                }

                SizeF card_size = SideCardSize;
                if (index == static_cast<int64>(m_selected_index) + direction) {
                    card_size = SideCardSize.lerp(SelectedCardSize, neighbor_scale);
                    x += direction * (CardSpacing + SelectedCardMargin) * neighbor_scale;
                }

                drawCard(
                    static_cast<std::size_t>(index),
                    RectF{ Arg::center = Vec2{ x + direction * card_size.x / 2.0, CardY }, card_size }
                );
                x += direction * (CardSpacing + SideCardSize.x);
            }
        };

        draw_side(1);
        draw_side(-1);
    }

    void Settings::drawCard(const std::size_t index, const RectF& region) const {
        region.drawShadow(Vec2{ 12, 26 }, 32, 0, ColorF{ 0, 0, 0, 0.22 });
        region(m_setting_card.render(m_setting_cards[index], theme::Palette::Purple)).draw();
    }

    void Settings::drawArrows() const {
        if (m_setting_cards.isEmpty()) {
            return;
        }

        constexpr Vec2 Center{ DesignSize.x / 2.0, CardY };
        constexpr Vec2 Right = Center.movedBy(SelectedCardSize.x / 2.0 - 10, 0);
        constexpr Vec2 Left = Center.movedBy(-SelectedCardSize.x / 2.0 + 10, 0);
        if (m_selected_index + 1 < m_setting_cards.size()) {
            primitives::DrawArrow(Right, primitives::ArrowDirection::Right, theme::Palette::Gray);
            primitives::DrawArrow(Right.movedBy(30, 0), primitives::ArrowDirection::Right, theme::Palette::Gray);
        }
        if (m_selected_index > 0) {
            primitives::DrawArrow(Left, primitives::ArrowDirection::Left, theme::Palette::Gray);
            primitives::DrawArrow(Left.movedBy(-30, 0), primitives::ArrowDirection::Left, theme::Palette::Gray);
        }
    }

    void Settings::RegisterAssets() {
        if (!TextureAsset::Register(
                Assets::Header,
                components::MakeMenuHeaderTexture(
                    U"SETTINGS",
                    localization::GetText(localization::TextId::SettingsPrompt)
                )
            ) ||
            !TextureAsset::Load(Assets::Header)) {
            throw Error{ U"Failed to register the Settings header texture." };
        }
    }
}
