#pragma once
#include "Common.hpp"
#include "core/types/Sheet.hpp"
#include "core/types/Difficulty.hpp"

namespace ui::theme {
    struct DifficultyTheme;
}

namespace ui::components {
    class Tile {
    public:
        Tile();
        ~Tile() = default;

        const MSRenderTexture& get(const core::types::SheetMetadata& data, const TextureRegion& jacket, int8 difficulty_index, const double offset = 0.0) const;

        static void RegisterAssets();

    private:
        MSRenderTexture m_tile_rt;

        void drawBackground(const ui::theme::DifficultyTheme& theme, const TextureRegion& jacket) const;
        void drawDifficultySection(const core::types::Difficulty difficulty, const int32 level, const ui::theme::DifficultyTheme& theme) const;
        void drawMetadataSection(const core::types::SheetMetadata& data, StringView designer, double offset, const ui::theme::DifficultyTheme& theme) const;
        void drawBadge(const Vec2& pos, const Vec2& size, StringView s, const ColorF& color) const;

        static constexpr Size TileSize{ 416, 545 };
        static constexpr Size SparkleSize{ 44, 61 };
        static constexpr int32 MockHighScore = 1006000; // TODO: get real high score

        struct Assets {
            static constexpr AssetNameView Sparkle{ U"Tile.Sparkle" };
        };
    };
}
