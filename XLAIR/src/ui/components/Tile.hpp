#pragma once
#include "Common.hpp"
#include "core/types/Sheet.hpp"

namespace ui::components {
    class Tile {
    public:
        Tile();
        ~Tile() = default;

        const MSRenderTexture& get(const core::types::SheetMetadata& data, const TextureRegion& jacket, size_t difficulty_index, const double offset = 0.0) const;

    private:
        MSRenderTexture m_tile_rt;

        void drawBadge(const Vec2& pos, const Vec2& size, StringView s) const;

        constexpr static int32 Scale { 2 };
        static constexpr Size TileSize { 416, 545 };
    };
}
