#include "Assets.hpp"

namespace app::assets {
    void Initialize() {
        InitializeFonts();
        InitializeShaders();
    }

    void InitializeFonts() {
        FontAsset::Register(font::UiLabel, FontMethod::MSDF, 38, U"assets/fonts/Jost/Jost-Medium.ttf");
        FontAsset::Load(font::UiLabel);

        FontAsset::Register(font::UiText, FontMethod::MSDF, 75, U"assets/fonts/Jost/Jost-Regular.ttf");
        FontAsset::Load(font::UiText);

        FontAsset::Register(U"font.cjk", FontMethod::MSDF, 75, Typeface::CJK_Regular_JP);
        FontAsset::Load(U"font.cjk");
        auto&& target_font = FontAsset(U"font.cjk");
        FontAsset(font::UiText).addFallback(target_font);

        FontAsset::Register(font::UiSubText, FontMethod::Bitmap, 68, U"assets/fonts/BrunoAce/BrunoAce-Regular.ttf");
        FontAsset::Load(font::UiSubText);
    }

    void InitializeShaders() {
        PixelShaderAsset::Register(shader::Grayscale, HLSL{ U"assets/shaders/grayscale.hlsl", U"PS" } | GLSL{ U"assets/shaders/grayscale.frag", {{U"PSConstants2D", 0}} });
        PixelShaderAsset::Load(shader::Grayscale);
    }
}
