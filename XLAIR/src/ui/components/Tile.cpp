#include "Tile.hpp"

namespace ui::components {
    Tile::Tile() : m_tile_rt { Scale * TileSize } {}

    const MSRenderTexture& Tile::get(const core::types::SheetMetadata& data, const TextureRegion& jacket, size_t level, const double offset) const {
        m_tile_rt.clear(Palette::White);

        constexpr ColorF AccentColor = ColorF { U"#FF8EFF" };
        constexpr ColorF SubColor = ColorF { U"#78D4FF" };
        constexpr ColorF TextColor = ColorF { U"#6C6E8D" };
        constexpr StringView LevelStr = U"ADVANCED";
        constexpr StringView TitleStr = U"六兆年と一夜物語";
        constexpr size_t LevelNum = 12;

        {
            ScopedRenderTarget2D target { m_tile_rt };

            RectF { Scale * TileSize }.drawFrame(Scale * 2, 0, AccentColor);

            RectF { Scale * Vec2{ 58, 17 }, Scale * Size{ 337, 337 } }.draw(AccentColor);

            RectF { Scale * Vec2{ 63, 17 }, Scale * Size{ 332, 332 } }(jacket).draw();

            TextureAsset(U"sparkle").resized(44, 60).scaled(Scale).draw(Scale * Vec2 { 8, 320 }, Arg::top = AccentColor, Arg::bottom = SubColor);
            TextureAsset(U"sparkle").resized(44, 60).scaled(Scale).draw(Scale * Vec2 { 8, 375 }, Arg::top = AccentColor, Arg::bottom = SubColor);
            TextureAsset(U"sparkle").resized(44, 60).scaled(Scale).draw(Scale * Vec2 { 8, 429 }, Arg::top = AccentColor, Arg::bottom = SubColor);

            RectF { Scale * Vec2{ 171, 426 }, Scale * Size{ 10, 2 } }.draw(AccentColor);
            RectF { Scale * Vec2{ 175, 428 }, Scale * Size{ 2, 6 } }.draw(AccentColor);

            RectF { Scale * Vec2{ 171, 487 }, Scale * Size{ 10, 2 } }.draw(AccentColor);
            RectF { Scale * Vec2{ 175, 481 }, Scale * Size{ 2, 6 } }.draw(AccentColor);

            {
                const Transformer2D tr { Mat3x2::Rotate(90_deg, Scale * Vec2{0, 0})};
                FontAsset(U"tile")(LevelStr).draw(Scale * 36, Scale * Vec2 { 18, -57 }, AccentColor);
            }

            {
                const Transformer2D tr { Mat3x2::Rotate(-90_deg, Scale * Vec2{0, 0})};
                FontAsset(U"tile")(U"LEVEL").draw(Scale * 21, Scale * Vec2 {-485, 65 }, AccentColor);
            }

            {
                FontAsset(U"tile.text")(LevelNum).drawBase(Scale * 75, Scale * 90, Scale * 483, TextColor);
            }

            {
                FontAsset(U"tile.text")(TitleStr).draw(Scale * 36, Arg::bottomLeft = Scale * Vec2{ 107, 408 }, TextColor);
            }

        }

        Graphics2D::Flush();
        m_tile_rt.resolve();
        return m_tile_rt;
    }
}
