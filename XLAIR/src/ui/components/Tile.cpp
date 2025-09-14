#include "Tile.hpp"

namespace ui::components {
    Tile::Tile() : m_tile_rt{ Scale * TileSize } {}

    const MSRenderTexture& Tile::get(const core::types::SheetMetadata& data, const TextureRegion& jacket, size_t difficulty_index, const double offset) const {
        const auto difficulty = data.difficulties[difficulty_index];

        constexpr ColorF AccentColor = ColorF{ U"#FF8EFF" };
        constexpr ColorF SubColor = ColorF{ U"#78D4FF" };
        constexpr ColorF TextColor = ColorF{ U"#6C6E8D" };
        constexpr ColorF SubTextColor = ColorF{ U"#9798AA" };
        constexpr StringView DifficultyStr = U"ADVANCED";
        constexpr int32 Score = 1006000;

        m_tile_rt.clear(Palette::White);
        {
            ScopedRenderTarget2D target{ m_tile_rt };

            // background
            {
                constexpr ColorF BG = (AccentColor * 1.4).withA(0.25);
                Circle{ Scale * Vec2{ 0, 0 }, Scale * 100 }.draw(BG, Palette::White);
                Circle{ Scale * Vec2{ TileSize.x, 0 }, Scale * 100 }.draw(BG, Palette::White);
                Circle{ Scale * Vec2{ 0, TileSize.y }, Scale * 100 }.draw(BG, Palette::White);
                Circle{ Scale * Vec2{ TileSize.x, TileSize.y }, Scale * 100 }.draw(BG, Palette::White);
            }

            // frame
            RectF{ Scale * TileSize }.drawFrame(Scale * 2, 0, AccentColor);

            // jacket
            RectF{ Scale * Vec2{ 58, 20 }, Scale * Size{ 338, 338 } }.draw(AccentColor);
            RectF{ Scale * Vec2{ 64, 20 }, Scale * Size{ 332, 332 } }(jacket).draw();

            // decorations
            Line{ Scale * Vec2{ 349, 10 }, Scale * Vec2{ 406, 10 } }.draw(Scale * 2, AccentColor);
            Line{ Scale * Vec2{ 406, 10 }, Scale * Vec2{ 406, 67 } }.draw(Scale * 2, AccentColor);

            Line{ Scale * Vec2{ 213, 534 }, Scale * Vec2{ 406, 534 } }.draw(Scale * 2, AccentColor);
            Line{ Scale * Vec2{ 406, 496 }, Scale * Vec2{ 406, 534 } }.draw(Scale * 2, AccentColor);

            TextureAsset(U"sparkle").resized(44, 60).scaled(Scale).draw(Scale * Vec2{ 8, 341 }, Arg::top = AccentColor, Arg::bottom = SubColor);
            TextureAsset(U"sparkle").resized(44, 60).scaled(Scale).draw(Scale * Vec2{ 8, 396 }, Arg::top = AccentColor, Arg::bottom = SubColor);
            TextureAsset(U"sparkle").resized(44, 60).scaled(Scale).draw(Scale * Vec2{ 8, 450 }, Arg::top = AccentColor, Arg::bottom = SubColor);

            // difficulty
            {
                const Transformer2D tr{ Mat3x2::Rotate(90_deg, Scale * Vec2{0, 0}) };
                FontAsset(U"tile")(DifficultyStr).draw(Scale * 36, Scale * Vec2{ 20, -57 }, AccentColor);
            }

            // level
            {
                const Transformer2D tr{ Mat3x2::Rotate(-90_deg, Scale * Vec2{0, 0}) };
                FontAsset(U"tile")(U"LEVEL").draw(Scale * 23, Scale * Vec2{ -505, 50 }, AccentColor);
            }
            {
                const double level_x = (difficulty.level < 10 ? 90 : 80);
                FontAsset(U"tile.text")(difficulty.level).drawBase(Scale * 87, Scale * level_x, Scale * 503, TextColor);
            }

            // title and artist
            {
                constexpr Rect DescriptionRegion{ Scale * Point{ 58, 363 }, Scale * Point{ 338, 75 } };
                constexpr double DescriptionOffsetMargin = Scale * 80;
                constexpr double DescriptionVel = Scale * 50.0;
                constexpr double TitleY = Scale * 50;
                constexpr auto TitleFotntSize = Scale * 36;
                constexpr double ArtistY = Scale * 75;
                constexpr auto ArtistFontSize = Scale * 24;

                const ScopedViewport2D viewport{ DescriptionRegion };

                const RectF title_region = FontAsset(U"tile.text")(data.title).region(TitleFotntSize);
                const RectF artist_region = FontAsset(U"tile.text")(data.artist).region(ArtistFontSize);

                if (title_region.w <= DescriptionRegion.w) {
                    const Vec2 pos{ DescriptionRegion.w / 2.0, TitleY };
                    FontAsset(U"tile.text")(data.title).draw(TitleFotntSize, Arg::bottomCenter = pos, TextColor);
                }
                else {
                    const double t = Math::Fmod(offset, (title_region.w + DescriptionOffsetMargin) / DescriptionVel);
                    FontAsset(U"tile.text")(data.title).draw(TitleFotntSize, Arg::bottomLeft(-t * DescriptionVel, TitleY), TextColor);
                    FontAsset(U"tile.text")(data.title).draw(TitleFotntSize, Arg::bottomLeft(title_region.w + DescriptionOffsetMargin - t * DescriptionVel, TitleY), TextColor);
                }

                if (artist_region.w <= DescriptionRegion.w) {
                    const Vec2 pos{ DescriptionRegion.w / 2.0, ArtistY };
                    FontAsset(U"tile.text")(data.artist).draw(ArtistFontSize, Arg::bottomCenter = pos, SubTextColor);
                }
                else {
                    const double t = Math::Fmod(offset, (artist_region.w + DescriptionOffsetMargin) / DescriptionVel);
                    FontAsset(U"tile.text")(data.artist).draw(ArtistFontSize, Arg::bottomLeft(-t * DescriptionVel, ArtistY), SubTextColor);
                    FontAsset(U"tile.text")(data.artist).draw(ArtistFontSize, Arg::bottomLeft(artist_region.w + DescriptionOffsetMargin - t * DescriptionVel, ArtistY), SubTextColor);
                }
            }

            // high score
            {
                FontAsset(U"tile")(U"HIGH").draw(Scale * 17, Scale * 177, Scale * 470, AccentColor);
                FontAsset(U"tile")(U"SCORE").draw(Scale * 17, Scale * 177, Scale * 485, AccentColor);

                FontAsset(U"tile.text")(Score).draw(Scale * 40, Arg::bottomRight = Scale * Vec2{ 395, 520 }, TextColor);
            }

            // badge
            {
                // Rank
                drawBadge(Scale * Vec2{ 177, 446 }, Scale * Vec2{ 74, 23 }, U"SSS");
                // Status
                drawBadge(Scale * Vec2{ 255, 446 }, Scale * Vec2{ 140, 23 }, U"FULL COMBO");
            }

            // notes designer
            FontAsset(U"tile.text")(U"NOTES DESIGNER: {}"_fmt(difficulty.designer)).drawBase(Scale * 15, Scale * 16, Scale * 534, AccentColor);
        }

        Graphics2D::Flush();
        m_tile_rt.resolve();
        return m_tile_rt;
    }

    void Tile::drawBadge(const Vec2& pos, const Vec2& size, StringView s) const {
        constexpr ColorF AccentColor = ColorF{ U"#FF8EFF" };
        constexpr ColorF SubColor = AccentColor.withA(0.3);
        const auto mid = size.withX(size.x * 0.75);
        const auto end = size.withX(size.x * 0.25);
        RectF{ pos, mid }.draw(Arg::left = AccentColor, SubColor);
        RectF{ pos.movedBy(mid.x, 0), end }.draw(Arg::left = SubColor, Arg::right = AccentColor);

        FontAsset(U"tile")(s).draw(Scale * 18, Arg::center = pos.movedBy(size.x / 2.0, size.y / 2.0), Palette::White);
    }
}
