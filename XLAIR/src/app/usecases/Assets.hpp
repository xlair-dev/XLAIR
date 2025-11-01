#pragma once
#include "Common.hpp"

namespace app::assets {
    namespace font {
        static constexpr inline AssetNameView UiLabel{ U"Font.UiLabel" };
        static constexpr inline AssetNameView UiText{ U"Font.UiText" };
        static constexpr inline AssetNameView UiSubText{ U"Font.UiSubText" };
        static constexpr inline AssetNameView UiComboNumber{ U"Font.ComboNumber" };
    }

    namespace shader {
        static constexpr inline AssetNameView Grayscale{ U"Shader.Grayscale" };
    }

    namespace texture {
        static constexpr inline AssetNameView Logo{ U"Texture.Logo" };
        static constexpr inline AssetNameView GameScoreBarFrame{ U"Texture.GameScoreBarFrame" };
        static constexpr inline AssetNameView ResultBackground{ U"Texture.ResultBackground" };
        static constexpr inline AssetNameView Perfect{ U"Texture.Perfect" };
        static constexpr inline AssetNameView Great{ U"Texture.Great" };
        static constexpr inline AssetNameView Good{ U"Texture.Good" };
        static constexpr inline AssetNameView Miss{ U"Texture.Miss" };
    }

    void Initialize();
    void InitializeFonts();
    void InitializeShaders();
    void InitializeTextures();
}
