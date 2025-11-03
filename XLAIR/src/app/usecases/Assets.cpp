#include "Assets.hpp"

namespace app::assets {
    void Initialize() {
        InitializeFonts();
        InitializeShaders();
        InitializeTextures();
        InitializeSounds();
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

        TextureAsset::Register(texture::GradeBack, U"assets/textures/grade-back.png");
        TextureAsset::Load(texture::GradeBack);

        TextureAsset::Register(texture::GradeS, U"assets/textures/grade-s.png");
        TextureAsset::Load(texture::GradeS);
        TextureAsset::Register(texture::GradeA, U"assets/textures/grade-a.png");
        TextureAsset::Load(texture::GradeA);
        TextureAsset::Register(texture::GradeB, U"assets/textures/grade-b.png");
        TextureAsset::Load(texture::GradeB);
    }

    void InitializeSounds() {
        AudioAsset::Register(sounds::se::MoveRightLeft, U"assets/sounds/se/move_right_left.wav");
        AudioAsset::Load(sounds::se::MoveRightLeft);
        AudioAsset::Register(sounds::se::ChangeLevel, U"assets/sounds/se/change_level.wav");
        AudioAsset::Load(sounds::se::ChangeLevel);
        AudioAsset::Register(sounds::se::SelectMusic, U"assets/sounds/se/select_music.wav");
        AudioAsset::Load(sounds::se::SelectMusic);
        AudioAsset::Register(sounds::se::Finish, U"assets/sounds/se/finish.wav");
        AudioAsset::Load(sounds::se::Finish);

        AudioAsset::Register(sounds::bgm::FinalResult, U"assets/sounds/bgm/final_result.wav");
        AudioAsset::Load(sounds::bgm::FinalResult);
    }
}
