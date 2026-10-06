#pragma once

#include "Common.hpp"

namespace xlair::ui::assets {
    namespace font {
        inline constexpr AssetNameView Label{ U"XLAIR.UI.Font.Label" };
        inline constexpr AssetNameView Text{ U"XLAIR.UI.Font.Text" };
        inline constexpr AssetNameView Display{ U"XLAIR.UI.Font.Display" };
        inline constexpr AssetNameView ComboNumber{ U"XLAIR.UI.Font.ComboNumber" };
        inline constexpr AssetNameView CjkFallback{ U"XLAIR.UI.Font.CjkFallback" };
    }

    namespace texture {
        inline constexpr AssetNameView Logo{ U"XLAIR.UI.Texture.Logo" };
        inline constexpr AssetNameView GameScoreBarFrame{ U"XLAIR.UI.Texture.GameScoreBarFrame" };
        inline constexpr AssetNameView SettingValueArrowEnabled{ U"XLAIR.UI.Texture.SettingValueArrowEnabled" };
        inline constexpr AssetNameView SettingValueArrowDisabled{ U"XLAIR.UI.Texture.SettingValueArrowDisabled" };
    }

    namespace sound {
        inline constexpr AssetNameView Navigate{ U"XLAIR.UI.Sound.Navigate" };
        inline constexpr AssetNameView ChangeDifficulty{ U"XLAIR.UI.Sound.ChangeDifficulty" };
        inline constexpr AssetNameView Confirm{ U"XLAIR.UI.Sound.Confirm" };
    }

    void Initialize();
}
