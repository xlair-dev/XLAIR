#pragma once
#include "Common.hpp"
#include "core/types/Sheet.hpp"

namespace ui::components {
    class Tile {
    public:
        Tile();
        ~Tile() = default;

        const MSRenderTexture& get(const core::types::SheetMetadata& data, const TextureRegion& jacket, size_t level, const double offset = 0.0) const;

    private:
        MSRenderTexture m_tile_rt;

        constexpr static int32 Scale { 2 };
        static constexpr Size TileSize { 416, 505 };
    };
}
