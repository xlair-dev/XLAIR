#include "Assets.hpp"

namespace app::assets {
    void Initialize() {
        InitializeFonts();
        InitializeShaders();
        InitializeTextures();
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

        FontAsset::Register(font::UiSubText, FontMethod::MSDF, 68, U"assets/fonts/BrunoAce/BrunoAce-Regular.ttf");
        FontAsset::Load(font::UiSubText);

        FontAsset::Register(font::UiComboNumber, FontMethod::Bitmap, 270, U"assets/fonts/Jost/Jost-Regular.ttf");
        FontAsset::Load(font::UiComboNumber, U"0123456789");
    }

    void InitializeShaders() {
        PixelShaderAsset::Register(shader::Grayscale, HLSL{ U"assets/shaders/grayscale.hlsl", U"PS" } | GLSL{ U"assets/shaders/grayscale.frag", {{U"PSConstants2D", 0}} });
        PixelShaderAsset::Load(shader::Grayscale);
    }

    void InitializeTextures() {
        TextureAsset::Register(texture::GameScoreBarFrame, U"assets/textures/bar.png");
        TextureAsset::Load(texture::GameScoreBarFrame);

        TextureAsset::Register(texture::Logo, U"assets/textures/logo.png");
        TextureAsset::Load(texture::Logo);

        TextureAsset::Register(texture::ResultBackground, U"assets/textures/result-background.png");
        TextureAsset::Load(texture::ResultBackground);

        TextureAsset::Register(texture::Perfect, U"assets/textures/perfect.png");
        TextureAsset::Load(texture::Perfect);
        TextureAsset::Register(texture::Great, U"assets/textures/great.png");
        TextureAsset::Load(texture::Great);
        TextureAsset::Register(texture::Good, U"assets/textures/good.png");
        TextureAsset::Load(texture::Good);
        TextureAsset::Register(texture::Miss, U"assets/textures/miss.png");
        TextureAsset::Load(texture::Miss);
    }
}
