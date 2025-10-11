#include "Assets.hpp"

namespace app::assets {
    void Initialize() {
        InitializeFonts();
        InitializeShaders();
    }

    void InitializeFonts() {
        FontAsset::Register(font::UiLabel, FontMethod::Bitmap, 38, U"assets/fonts/Jost/Jost-Medium.ttf");
        FontAsset::Load(font::UiLabel);

        FontAsset::Register(font::UiText, FontMethod::MSDF, 75, U"assets/fonts/Jost/Jost-Regular.ttf");
        FontAsset::Load(font::UiText);

        const Font cjk{ FontMethod::MSDF, 75, Typeface::CJK_Regular_JP };
        FontAsset(font::UiText).addFallback(cjk);

        FontAsset::Register(font::UiTimer, FontMethod::Bitmap, 68, U"assets/fonts/BrunoAce/BrunoAce-Regular.ttf");
        FontAsset::Load(font::UiTimer);
    }

    void InitializeShaders() {
        PixelShaderAsset::Register(shader::Grayscale, HLSL{ U"assets/shaders/grayscale.hlsl", U"PS" } | GLSL{ U"assets/shaders/grayscale.frag", {{U"PSConstants2D", 0}} });
        PixelShaderAsset::Load(shader::Grayscale);
    }
}
