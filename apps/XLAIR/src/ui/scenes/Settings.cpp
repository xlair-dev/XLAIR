#include "Settings.hpp"

#include "core/user/Level.hpp"
#include "ui/Design.hpp"
#include "ui/assets/Assets.hpp"
#include "ui/audio/SoundEffect.hpp"
#include "ui/components/MenuHeader.hpp"
#include "ui/components/MenuTimerPlate.hpp"
#include "ui/components/UserNameplate.hpp"
#include "ui/input/SliderInput.hpp"
#include "ui/localization/Localization.hpp"
#include "ui/theme/Palette.hpp"

#include <cmath>

namespace xlair::ui::scenes {
    namespace {
        constexpr double CardY = 553.0;

        // Provisional bounds for the UI prototype, not the final game-option constraints.
        constexpr double MinimumSpeed = 0.25;
        constexpr double MaximumSpeed = 10.0;
        constexpr double SpeedStep = 0.25;
        constexpr int32 MinimumJudgmentOffsetMs = -100;
        constexpr int32 MaximumJudgmentOffsetMs = 100;

        [[nodiscard]]
        Array<components::SettingCardData> MakeSettingCards() {
            return {
                {
                    .title = localization::GetText(localization::TextId::SettingsCardSpeedTitle),
                    .description = localization::GetText(localization::TextId::SettingsCardSpeedDescription),
                    .value = U"1.00",
                },
                {
                    .title = localization::GetText(localization::TextId::SettingsCardJudgmentOffsetTitle),
                    .description = localization::GetText(localization::TextId::SettingsCardJudgmentOffsetDescription),
                    .value = U"+0",
                },
                {
                    .title = localization::GetText(localization::TextId::SettingsCardMirrorTitle),
                    .description = localization::GetText(localization::TextId::SettingsCardMirrorDescription),
                    .value = U"OFF",
                    .can_decrease = false,
                    .show_arrow_labels = false,
                },
            };
        }

        [[nodiscard]]
        Array<components::SliderMapping> MakeSliderMappings() {
            return {
                {
                    .region = { .start = 0, .width = 3, .right_corner = false },
                    .label = U"◀ {}"_fmt(localization::GetText(localization::TextId::SettingsSliderMoveLeft)),
                    .color = theme::Palette::Pink,
                },
                {
                    .region = { .start = 3, .width = 3, .left_corner = false },
                    .label = U"{} ▶"_fmt(localization::GetText(localization::TextId::SettingsSliderMoveRight)),
                    .color = theme::Palette::Pink,
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
        : SceneBase{ init },
          m_card_carousel{ components::SettingCard::size(), Vec2{ DesignSize.x / 2.0, CardY }, DesignSize.x },
          m_setting_cards{ MakeSettingCards() }, m_slider_mappings{ MakeSliderMappings() } {
        const auto& session = getData().application->playSession();
        const auto& options = session.playOptions();
        m_note_speed = std::isfinite(options.note_speed) ? options.note_speed : 1.0;
        m_judgment_offset_ms = options.judgment_offset_ms;
        m_mirror = session.mirror();
        updateCardValues();

        auto& context = getData().ensureMusicSelectContext();
        if (!m_setting_cards.isEmpty()) {
            m_selected_index = Min(context.selectedSettingIndex(), m_setting_cards.size() - 1);
            context.setSelectedSettingIndex(m_selected_index);
        }
    }

    void Settings::update() {
        handleInput();
        updatePresentation();
    }

    void Settings::updateFadeIn(const double) {
        updatePresentation();
    }

    void Settings::updateFadeOut(const double) {
        updatePresentation();
    }

    void Settings::updatePresentation() {
        m_card_carousel.update(Scene::DeltaTime());
    }

    void Settings::handleInput() {
        const auto* controller = getData().application->controller();
        if (KeyTab.down() || input::TouchRegionDown(controller, 14, 2)) {
            audio::PlaySoundEffect(audio::SoundEffect::Navigate);
            changeScene(SceneState::MusicSelect, transitions::CardTransitionMillisec, CrossFade::No);
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
        getData().music_select_context->setSelectedSettingIndex(m_selected_index);
        m_card_carousel.animateMove(direction);
        return true;
    }

    bool Settings::adjustValue(const int32 direction) {
        if (m_setting_cards.isEmpty() || direction == 0) {
            return false;
        }

        auto& card = m_setting_cards[m_selected_index];
        if (direction < 0 ? !card.can_decrease : !card.can_increase) {
            return false;
        }

        const int32 step = direction < 0 ? -1 : 1;
        auto& session = getData().application->playSession();
        switch (static_cast<SettingItem>(m_selected_index)) {
            case SettingItem::Speed: {
                // Move to the next quarter step even if the API value is off the UI grid.
                const double next_step =
                    step > 0 ? std::floor(m_note_speed / SpeedStep) + 1.0 : std::ceil(m_note_speed / SpeedStep) - 1.0;
                m_note_speed = Clamp(next_step * SpeedStep, MinimumSpeed, MaximumSpeed);
                session.playOptions().note_speed = m_note_speed;
                break;
            }
            case SettingItem::JudgmentOffset: {
                const int64 next = static_cast<int64>(m_judgment_offset_ms) + step;
                m_judgment_offset_ms = static_cast<int32>(Clamp(
                    next,
                    static_cast<int64>(MinimumJudgmentOffsetMs),
                    static_cast<int64>(MaximumJudgmentOffsetMs)
                ));
                session.playOptions().judgment_offset_ms = m_judgment_offset_ms;
                break;
            }
            case SettingItem::Mirror: {
                m_mirror = direction > 0;
                session.setMirror(m_mirror);
                break;
            }
            default:
                return false;
        }
        updateCardValues();
        return true;
    }

    void Settings::updateCardValues() {
        auto& speed = m_setting_cards[static_cast<std::size_t>(SettingItem::Speed)];
        speed.value = U"{:.2f}"_fmt(m_note_speed);
        speed.can_decrease = m_note_speed > MinimumSpeed;
        speed.can_increase = m_note_speed < MaximumSpeed;

        auto& judgment = m_setting_cards[static_cast<std::size_t>(SettingItem::JudgmentOffset)];
        judgment.value = U"{:+}"_fmt(m_judgment_offset_ms);
        judgment.can_decrease = m_judgment_offset_ms > MinimumJudgmentOffsetMs;
        judgment.can_increase = m_judgment_offset_ms < MaximumJudgmentOffsetMs;

        auto& mirror = m_setting_cards[static_cast<std::size_t>(SettingItem::Mirror)];
        mirror.value = m_mirror ? U"ON" : U"OFF";
        mirror.can_decrease = m_mirror;
        mirror.can_increase = !m_mirror;
    }

    void Settings::draw() const {
        drawScene({});
    }

    void Settings::drawFadeIn(const double t) const {
        drawScene(transitions::CardFadeIn(t));
    }

    void Settings::drawFadeOut(const double t) const {
        drawScene(transitions::CardFadeOut(t, transitions::CardZoom::Shrink));
    }

    void Settings::drawScene(const transitions::CardTransition& transition) const {
        Scene::Rect().draw(theme::Palette::White);

        if (transition.opacity > 0.0 && transition.scale > 0.0) {
            drawCards(transition);
            m_card_carousel.drawArrows(
                m_selected_index,
                m_setting_cards.size(),
                theme::Palette::Gray.withA(transition.opacity),
                transition.scale
            );
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

    void Settings::drawCards(const transitions::CardTransition& transition) const {
        for (const auto& placement :
             m_card_carousel.layout(m_selected_index, m_setting_cards.size(), transition.scale)) {
            drawCard(placement.index, placement.region, transition);
        }
    }

    void Settings::drawCard(
        const std::size_t index,
        const RectF& region,
        const transitions::CardTransition& transition
    ) const {
        region.drawShadow(
            Vec2{ 12, 26 } * transition.scale,
            32 * transition.scale,
            0,
            ColorF{ 0, 0, 0, 0.22 * transition.opacity }
        );
        // Keep offscreen rendering untouched and fade only the finished card texture.
        region(m_setting_card.render(m_setting_cards[index])).draw(ColorF{ 1.0, transition.opacity });
    }

    void Settings::RegisterAssets() {
        String latin_chars = U"0123456789.+-";
        String cjk_chars;
        const auto append_chars = [&](const StringView text) {
            for (const char32 ch : text) {
                (ch < 0x80 ? latin_chars : cjk_chars).push_back(ch);
            }
        };
        for (const auto& card : MakeSettingCards()) {
            append_chars(card.title);
            append_chars(card.description);
            append_chars(card.value);
        }
        if (!FontAsset{ assets::font::Text }.preload(latin_chars.sorted_and_uniqued()) ||
            !FontAsset{ assets::font::CjkFallback }.preload(cjk_chars.sorted_and_uniqued())) {
            throw Error{ U"Failed to preload the Settings card glyphs." };
        }

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
