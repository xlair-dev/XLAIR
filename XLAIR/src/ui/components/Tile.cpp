#include "Tile.hpp"
#include "ui/primitives/SparkleShape.hpp"

namespace ui::components {
    Tile::Tile() : m_tile_rt{ TileSize } {}

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
                Circle{ Vec2{ 0, 0 }, 100 }.draw(BG, Palette::White);
                Circle{ Vec2{ TileSize.x, 0 }, 100 }.draw(BG, Palette::White);
                Circle{ Vec2{ 0, TileSize.y }, 100 }.draw(BG, Palette::White);
                Circle{ Vec2{ TileSize.x, TileSize.y }, 100 }.draw(BG, Palette::White);
            }

            // frame
            RectF{ TileSize }.drawFrame(2, 0, AccentColor);

            // jacket
            RectF{ Vec2{ 58, 20 },  Size{ 338, 338 } }.draw(AccentColor);
            RectF{ Vec2{ 64, 20 },  Size{ 332, 332 } }(jacket).draw();

            // decorations
            Line{ Vec2{ 349, 10 },  Vec2{ 406, 10 } }.draw(2, AccentColor);
            Line{ Vec2{ 406, 10 },  Vec2{ 406, 67 } }.draw(2, AccentColor);

            Line{ Vec2{ 213, 534 },  Vec2{ 406, 534 } }.draw(2, AccentColor);
            Line{ Vec2{ 406, 496 },  Vec2{ 406, 534 } }.draw(2, AccentColor);

            TextureAsset(Assets::Sparkle).draw(Vec2{ 8, 341 }, Arg::top = AccentColor, Arg::bottom = SubColor);
            TextureAsset(Assets::Sparkle).draw(Vec2{ 8, 396 }, Arg::top = AccentColor, Arg::bottom = SubColor);
            TextureAsset(Assets::Sparkle).draw(Vec2{ 8, 450 }, Arg::top = AccentColor, Arg::bottom = SubColor);

            // difficulty
            {
                const Transformer2D tr{ Mat3x2::Rotate(90_deg, Vec2{0, 0}) };
                FontAsset(U"tile")(DifficultyStr).draw(36, Vec2{ 20, -57 }, AccentColor);
            }

            // level
            {
                const Transformer2D tr{ Mat3x2::Rotate(-90_deg, Vec2{0, 0}) };
                FontAsset(U"tile")(U"LEVEL").draw(23, Vec2{ -505, 50 }, AccentColor);
            }
            {
                const double level_x = (difficulty.level < 10 ? 90 : 80);
                FontAsset(U"tile.text")(difficulty.level).drawBase(87, level_x, 503, TextColor);
            }

            // title and artist
            {
                constexpr Rect DescriptionRegion{ Point{ 58, 363 },  Point{ 338, 75 } };
                constexpr double DescriptionOffsetMargin = 80;
                constexpr double DescriptionVel = 50.0;
                constexpr double TitleY = 50;
                constexpr auto TitleFotntSize = 36;
                constexpr double ArtistY = 75;
                constexpr auto ArtistFontSize = 24;

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
                FontAsset(U"tile")(U"HIGH").draw(17, 177, 470, AccentColor);
                FontAsset(U"tile")(U"SCORE").draw(17, 177, 485, AccentColor);

                FontAsset(U"tile.text")(Score).draw(40, Arg::bottomRight = Vec2{ 395, 520 }, TextColor);
            }

            // badge
            {
                // Rank
                drawBadge(Vec2{ 177, 446 }, Vec2{ 74, 23 }, U"SSS");
                // Status
                drawBadge(Vec2{ 255, 446 }, Vec2{ 140, 23 }, U"FULL COMBO");
            }

            // notes designer
            FontAsset(U"tile.text")(U"NOTES DESIGNER: {}"_fmt(difficulty.designer)).drawBase(15, 16, 534, AccentColor);
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

        FontAsset(U"tile")(s).draw(18, Arg::center = pos.movedBy(size.x / 2.0, size.y / 2.0), Palette::White);
    }

    std::unique_ptr<TextureAssetData> MakeSparkle() {
        std::unique_ptr<TextureAssetData> data = std::make_unique<TextureAssetData>();

        data->onLoad = [](TextureAssetData& asset, const String&) {
            const int32 scale = 3;
            static constexpr Size SparkleSize{ 44, 61 };
            const auto [w, h] = SparkleSize;
            Image image{ scale * SparkleSize };
            primitives::Sparkle(scale * Vec2{ w / 2.0, h / 2.0 }, scale * w / 2.0, scale * h / 2.0, 0).asPolygon().overwrite(image, Palette::White);

            for (int32 x = 0; x < image.width(); ++x) {
                for (int32 y = 0; y < image.height(); ++y) {
                    image[y][x] = Color{ Palette::White, image[y][x].a };
                }
            }

            asset.texture = Texture{ image.scaled(1.0 / scale, InterpolationAlgorithm::Area), TextureDesc::Mipped };
            return static_cast<bool>(asset.texture);
            };

        return data;
    }


    void Tile::RegisterAssets() {
        TextureAsset::Register({ Assets::Sparkle }, MakeSparkle());
        TextureAsset::Load({ Assets::Sparkle });
    }
}
