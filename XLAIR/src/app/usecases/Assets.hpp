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

    namespace sounds {
        namespace se {
            static constexpr inline AssetNameView MoveRightLeft{ U"Sound.SE.MoveRightLeft" };
            static constexpr inline AssetNameView SelectMusic{ U"Sound.SE.SelectMusic" };
            static constexpr inline AssetNameView ChangeLevel{ U"Sound.SE.ChangeLevel" };
            //static constexpr inline AssetNameView Tap{ U"Sound.SE.Tap" };
            //static constexpr inline AssetNameView XTap{ U"Sound.SE.XTap" };
            //static constexpr inline AssetNameView Flick{ U"Sound.SE.Flick" };
            //static constexpr inline AssetNameView Hold{ U"Sound.SE.Hold" };
            //static constexpr inline AssetNameView Release{ U"Sound.SE.Release" };
            //static constexpr inline AssetNameView JudgePerfect{ U"Sound.SE.Judge.Perfect" };
            //static constexpr inline AssetNameView JudgeGreat{ U"Sound.SE.Judge.Great" };
            //static constexpr inline AssetNameView JudgeGood{ U"Sound.SE.Judge.Good" };
            //static constexpr inline AssetNameView JudgeMiss{ U"Sound.SE.Judge.Miss" };
        }
    }

    void Initialize();
    void InitializeFonts();
    void InitializeShaders();
    void InitializeTextures();
    void InitializeSounds();
}
