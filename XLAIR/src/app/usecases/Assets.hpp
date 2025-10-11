#pragma once
#include "Common.hpp"

namespace app::assets {
    namespace font {
        static constexpr inline AssetNameView UiLabel{ U"Font.UiLabel" };
        static constexpr inline AssetNameView UiText{ U"Font.UiText" };
        static constexpr inline AssetNameView UiTimer{ U"Font.UiTimer" };
    }

    namespace shader {
        static constexpr inline AssetNameView Grayscale{ U"Shader.Grayscale" };
    }

    void Initialize();
    void InitializeFonts();
    void InitializeShaders();
}
