#include "Settings.hpp"

#include "core/user/Level.hpp"
#include "ui/components/MenuHeader.hpp"
#include "ui/components/MenuTimerPlate.hpp"
#include "ui/components/UserNameplate.hpp"
#include "ui/localization/Localization.hpp"
#include "ui/theme/Palette.hpp"

namespace xlair::ui::scenes {
    namespace {
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

    Settings::Settings(const InitData& init) : SceneBase{ init }, m_slider_mappings{ MakeSliderMappings() } {
        const auto& config = getData().application->config();
        if (config) {
            m_remaining_seconds = config->system.menu_timer_seconds;
        }
    }

    void Settings::update() {
        m_remaining_seconds = Max(0.0, m_remaining_seconds - Scene::DeltaTime());
    }

    void Settings::draw() const {
        Scene::Rect().draw(theme::Palette::White);

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

    void Settings::RegisterAssets() {
        if (!TextureAsset::Register(
                Assets::Header,
                components::MakeMenuHeaderTexture(
                    U"SETTING",
                    localization::GetText(localization::TextId::SettingsPrompt)
                )
            ) ||
            !TextureAsset::Load(Assets::Header)) {
            throw Error{ U"Failed to register the Settings header texture." };
        }
    }
}
