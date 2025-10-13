#include "Tile.hpp"
#include "app/usecases/Assets.hpp"
#include "ui/primitives/SparkleShape.hpp"
#include "ui/theme/DifficultyTheme.hpp"
#include "ui/components/ScrollingText.hpp"

namespace ui::components {
    Tile::Tile() : m_tile_rt{ TileSize } {}

    const MSRenderTexture& Tile::get(const core::types::SheetMetadata& data, const TextureRegion& jacket, int8 difficulty_index, const double offset) const {
        const auto difficulty = core::types::DifficultyFromInt(difficulty_index);
        const auto difficulty_info = data.difficulties[difficulty_index];
        const auto theme = theme::GetDifficultyTheme(difficulty);

        m_tile_rt.clear(Palette::White);
        {
            ScopedRenderTarget2D target{ m_tile_rt };

            drawBackground(theme, jacket);
            drawDifficultySection(difficulty, difficulty_info.level, theme);
            drawMetadataSection(data, difficulty_info.designer, offset, theme);

        }

        Graphics2D::Flush();
        m_tile_rt.resolve();
        return m_tile_rt;
    }

    void Tile::drawBackground(const ui::theme::DifficultyTheme& theme, const TextureRegion& jacket) const {
        const ColorF bg = (theme.accent * 1.5).withA(0.2);
        Circle{ Vec2{ 0, 0 }, 70 }.draw(bg, Palette::White);
        Circle{ Vec2{ TileSize.x, 0 }, 70 }.draw(bg, Palette::White);
        Circle{ Vec2{ 0, TileSize.y }, 70 }.draw(bg, Palette::White);
        Circle{ Vec2{ TileSize.x, TileSize.y }, 70 }.draw(bg, Palette::White);

        // frame
        RectF{ TileSize }.drawFrame(2, 0, theme.accent);

        // jacket
        RectF{ Vec2{ 58, 20 },  Size{ 338, 338 } }.draw(theme.accent);
        RectF{ Vec2{ 64, 20 },  Size{ 332, 332 } }(jacket).draw();

        // decorations
        Line{ Vec2{ 349, 10 },  Vec2{ 406, 10 } }.draw(2, theme.accent);
        Line{ Vec2{ 406, 10 },  Vec2{ 406, 67 } }.draw(2, theme.accent);

        Line{ Vec2{ 213, 534 },  Vec2{ 406, 534 } }.draw(2, theme.accent);
        Line{ Vec2{ 406, 496 },  Vec2{ 406, 534 } }.draw(2, theme.accent);

        TextureAsset(Assets::Sparkle).draw(Vec2{ 8, 341 }, Arg::top = theme.accent, Arg::bottom = theme.sub);
        TextureAsset(Assets::Sparkle).draw(Vec2{ 8, 396 }, Arg::top = theme.accent, Arg::bottom = theme.sub);
        TextureAsset(Assets::Sparkle).draw(Vec2{ 8, 450 }, Arg::top = theme.accent, Arg::bottom = theme.sub);
    }

    void Tile::drawDifficultySection(const core::types::Difficulty difficulty, const int32 level, const ui::theme::DifficultyTheme& theme) const {
        const auto difficulty_str = core::types::ToString(difficulty).uppercase();
        {
            const Transformer2D tr{ Mat3x2::Rotate(90_deg, Vec2{0, 0}) };
            FontAsset(app::assets::font::UiLabel)(difficulty_str).draw(36, Vec2{ 20, -57 }, theme.accent);
        }

        // level
        {
            const Transformer2D tr{ Mat3x2::Rotate(-90_deg, Vec2{0, 0}) };
            FontAsset(app::assets::font::UiLabel)(U"LEVEL").draw(23, Vec2{ -505, 50 }, theme.accent);
        }

        const double level_x = (level < 10 ? 90 : 80);
        FontAsset(app::assets::font::UiText)(level).drawBase(87, level_x, 503, theme.text);
    }

    void Tile::drawMetadataSection(const core::types::SheetMetadata& data, StringView designer, double offset, const ui::theme::DifficultyTheme& theme) const {
        // title and artist
        {
            auto&& font = FontAsset(app::assets::font::UiText);

            constexpr Rect TitleRegion{ Point{ 58, 363 },  Point{ 338, 50 } };
            constexpr Rect ArtistRegion{ Point{ 58, 363 },  Point{ 338, 75 } };

            DrawScrollingText(font, data.title, 36, TitleRegion, offset, theme.text, true);
            DrawScrollingText(font, data.artist, 24, ArtistRegion, offset, theme.sub_text, true);
        }

        // high score
        FontAsset(app::assets::font::UiLabel)(U"HIGH").draw(17, 177, 470, theme.accent);
        FontAsset(app::assets::font::UiLabel)(U"SCORE").draw(17, 177, 485, theme.accent);
        FontAsset(app::assets::font::UiText)(MockHighScore).draw(40, Arg::bottomRight = Vec2{ 395, 520 }, theme.text);

        // rank badge
        drawBadge(Vec2{ 177, 446 }, Vec2{ 74, 23 }, U"SSS", theme.accent);
        // status badge
        drawBadge(Vec2{ 255, 446 }, Vec2{ 140, 23 }, U"FULL COMBO", theme.accent);

        // notes designer
        FontAsset(app::assets::font::UiText)(U"NOTES DESIGNER: {}"_fmt(designer)).drawBase(15, 16, 534, theme.accent);
    }

    void Tile::drawBadge(const Vec2& pos, const Vec2& size, StringView s, const ColorF& color) const {
        const ColorF lighten = color.withA(0.3);
        const auto mid = size.withX(size.x * 0.75);
        const auto end = size.withX(size.x * 0.25);
        RectF{ pos, mid }.draw(Arg::left = color, lighten);
        RectF{ pos.movedBy(mid.x, 0), end }.draw(Arg::left = lighten, Arg::right = color);

        FontAsset(app::assets::font::UiLabel)(s).draw(18, Arg::center = pos.movedBy(size.x / 2.0, size.y / 2.0), Palette::White);
    }

    std::unique_ptr<TextureAssetData> MakeSparkle() {
        std::unique_ptr<TextureAssetData> data = std::make_unique<TextureAssetData>();

        data->onLoad = [](TextureAssetData& asset, const String&) {
            const int32 scale = 3;
            static constexpr Size SparkleSize{ 44, 61 };
            const auto [w, h] = SparkleSize;
            Image image{ scale * SparkleSize };
            primitives::Sparkle(scale * Vec2{ w / 2.0, h / 2.0 }, scale * w / 2.0, scale * h / 2.0, 0).asPolygon().overwrite(image, Palette::White);

            // avoid black outlines
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
