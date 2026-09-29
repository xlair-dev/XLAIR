#include "Settings.hpp"

#include "core/user/Level.hpp"
#include "ui/Design.hpp"
#include "ui/audio/SoundEffect.hpp"
#include "ui/components/MenuHeader.hpp"
#include "ui/components/MenuTimerPlate.hpp"
#include "ui/components/UserNameplate.hpp"
#include "ui/input/SliderInput.hpp"
#include "ui/localization/Localization.hpp"
#include "ui/theme/Palette.hpp"

namespace xlair::ui::scenes {
    namespace {
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
        const auto* controller = getData().application->controller();
        if (KeyTab.down() || input::TouchRegionDown(controller, 14, 2)) {
            audio::PlaySoundEffect(audio::SoundEffect::Navigate);
            changeScene(SceneState::MusicSelect, 0);
        }
    }

    void Settings::draw() const {
        Scene::Rect().draw(theme::Palette::White);

        drawCards();

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
        constexpr Size CardSize = components::SettingCard::size();
        constexpr double CardSpacing = 50.0;
        constexpr double CardY = 553.0;
        const double total_width = m_setting_cards.size() * (CardSize.x + CardSpacing) - CardSpacing;
        const double first_center_x = (DesignSize.x - total_width + CardSize.x) / 2.0;

        for (std::size_t index = 0; index < m_setting_cards.size(); ++index) {
            const RectF region{
                Arg::center = Vec2{ first_center_x + index * (CardSize.x + CardSpacing), CardY },
                CardSize,
            };
            region.drawShadow(Vec2{ 12, 26 }, 32, 0, ColorF{ 0, 0, 0, 0.22 });
            region(m_setting_card.render(m_setting_cards[index], theme::Palette::Purple)).draw();
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
