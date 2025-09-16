#include "MusicSelect.hpp"

#include "app/consts/Scene.hpp"

#include "ui/components/Tile.hpp"
#include "ui/components/PlayerNameplate.hpp"
#include "ui/components/MenuTimerPlate.hpp"
#include "ui/primitives/Arrow.hpp"
#include "ui/theme/Palette.hpp"

namespace ui {
    using namespace app::consts;
    using namespace ui::consts;

    MusicSelect::MusicSelect(const InitData& init) : IScene(init) {}

    MusicSelect::~MusicSelect() {}

    void MusicSelect::update() {
        // TODO: input handling in infra layer
        if (m_selected_index + 1 < getData().sheetRepository.size()) {
            if (KeyRight.down()) {
                ++m_selected_index;
                m_scroll_offset = -1.0;
                m_tile_offset_raw = -OffsetWait;
            }
        }
        if (m_selected_index > 0) {
            if (KeyLeft.down()) {
                --m_selected_index;
                m_scroll_offset = 1.0;
                m_tile_offset_raw = -OffsetWait;
            }
        }

        m_scroll_offset = Math::SmoothDamp(m_scroll_offset, 0.0, m_scroll_velocity, 0.1);
        m_tile_offset_raw += Scene::DeltaTime();
        m_tile_offset = Math::Max(0.0, m_tile_offset_raw);
    }

    void MusicSelect::draw() const {

        // drawBackground();

        drawUI();

        drawTiles();

        drawArrows();

        components::DrawPlayerNameplate(Point{ 59, 72 });
        components::DrawMenuTimerPlate(Point{ 1599, 72 }, 58, 1);

    }

    void MusicSelect::drawBackground() const {
        const auto& repo = getData().sheetRepository;
        // TODO: add change animation
        const ScopedCustomShader2D shader{ PixelShaderAsset(U"grayscale") };
        RectF{ SceneSize.maxComponent() }(*repo.getJacket(m_selected_index)).draw(ColorF{ 1.0, 1.0, 1.0, 0.05 });
    }

    void MusicSelect::drawUI() const {
        TextureAsset(Assets::MenuUITitle).drawAt(SceneWidth / 2.0, 153);

        // constants
        constexpr double LineWidth = 177;
        constexpr double SideOffset = 397.0;
        constexpr double Y1 = 130.0;
        constexpr double Y2 = 147.0;

        // color stops
        constexpr ColorF C0{ ColorF{ U"#9B9FCF" }, 0.00 };
        constexpr ColorF C1{ ColorF{ U"#79C8F3" }, 0.94 };
        constexpr ColorF C2{ ColorF{ U"#EFEEFE" }, 1.00 };
        constexpr ColorF C3{ ColorF{ U"#FF8EFF" }, 0.00 };

        const auto drawBar = [&](double base, double dir) {
            const double T0 = base;
            const double T1 = base + dir * (LineWidth * 0.06);
            const double T2 = base + dir * (LineWidth * 0.86);
            const double T3 = base + dir * LineWidth;

            for (const double y : { Y1, Y2 }) {
                Line{ T0, y, T1, y }.draw(3, C0, C1);
                Line{ T1, y, T2, y }.draw(3, C1, C2);
                Line{ T2, y, T3, y }.draw(3, C2, C3);
            }
        };

        drawBar(SideOffset + LineWidth, -1.0); // left
        drawBar(SceneWidth - SideOffset - LineWidth, 1.0); // right
    }

    void MusicSelect::drawTiles() const {
        const auto& repo = getData().sheetRepository;

        constexpr Vec2 center{ SceneWidth / 2.0, TileY };

        // scroll value (right:+ / left:-)
        const double s = m_scroll_offset;
        const double s_abs = Math::Abs(s);

        // selected tile
        const SizeF selected_tile_size = SelectedTileSize.lerp(TileSize, s_abs);
        const double neighbor_gap = (SelectedTileSize.x / 2.0 + TileSize.x / 2.0 + TileSpacing + SelectedTileMargin);
        const double selected_tile_x = center.x - neighbor_gap * s;

        const RectF selected_tile{ Arg::center = Vec2{ selected_tile_x, TileY }, selected_tile_size };

        selected_tile.drawShadow(Vec2{ 12, 26 }, 32.0, 0, ColorF{ 0, 0, 0, 0.22 });
        selected_tile(
            m_tile.get(repo.getMetadata(m_selected_index), *repo.getJacket(m_selected_index), 0, m_tile_offset)
        ).draw();

        const auto drawSide = [&](int32 dir) {
            const double ds = dir * s;
            const double margin_factor = Math::Min(1.0, 1.0 + ds);
            const double neighbor_scale = Math::Clamp(ds, 0.0, 1.0);
            
            double x = selected_tile_x + dir * (selected_tile_size.x / 2.0 + TileSpacing + SelectedTileMargin * margin_factor);

            const int32 start = static_cast<int32>(m_selected_index) + dir;
            const int32 end_cmp = (dir > 0 ? static_cast<int32>(repo.size()) : -1);

            for (int32 i = start; (dir > 0) ? (i < end_cmp) : (i > end_cmp); i += dir) {
                if (x - dir * TileSize.x > SceneWidth or x - dir * TileSize.x < 0) {
                    // out of screen
                    break;
                }

                const Vec2 tile_pos{ x, TileY };
                SizeF tile_size = TileSize;

                if (i == static_cast<int32>(m_selected_index) + dir) {
                    tile_size = TileSize.lerp(SelectedTileSize, neighbor_scale);
                    x += dir * (TileSpacing + SelectedTileMargin) * neighbor_scale;
                }

                RectF tile{ Arg::center = tile_pos.movedBy(dir * tile_size.x / 2, 0), tile_size};

                tile.drawShadow(Vec2{ 12, 26 }, 32.0, 0, ColorF{ 0, 0, 0, 0.22 });
                tile(
                    m_tile.get(repo.getMetadata(i), *repo.getJacket(i), 0)
                ).draw();

                x += dir * (TileSpacing + TileSize.x);
            }
        };

        drawSide(+1);  // right side
        drawSide(-1); // left side
    }

    void MusicSelect::drawArrows() const {
        constexpr ColorF color{ U"#7E7E7E" };
        constexpr Vec2 center{ SceneWidth / 2.0, TileY };

        constexpr Vec2 right_center = center.movedBy(SelectedTileSize.x / 2.0 - 10, 0);
        constexpr Vec2 left_center = center.movedBy(-SelectedTileSize.x / 2.0 + 10, 0);

        primitives::DrawArrow(right_center, color, +1);
        primitives::DrawArrow(right_center.movedBy(30, 0), color, +1);

        primitives::DrawArrow(left_center, color, -1);
        primitives::DrawArrow(left_center.movedBy(-30, 0), color, -1);
    }

    std::unique_ptr<TextureAssetData> MakeMenuUITitle() {
        std::unique_ptr<TextureAssetData> data = std::make_unique<TextureAssetData>();

        data->onLoad = [](TextureAssetData& asset, const String&) {
            constexpr StringView TextMain = U"MUSIC SELECT";
            constexpr StringView TextSub = U"楽曲を選択してください";

            Font font_main{ 70, U"assets/fonts/BrunoAce/BrunoAce-Regular.ttf" };
            const auto region_main = font_main(TextMain).region(70).asRect();

            Font font_sub{ 24, Typeface::CJK_Regular_JP };
            const auto region_sub = font_sub(TextSub).region(24).asRect();

            Image image{ static_cast<size_t>(region_main.w), static_cast<size_t>(region_main.h + region_sub.h) };
            font_main(TextMain).stamp(image, 0, 0);

            for (int32 x = 0; x < image.width(); ++x) {
                const double t = static_cast<double>(x) / image.width();
                constexpr ColorF left = theme::Palette::Pink;
                constexpr ColorF right = theme::Palette::Cyan;
                for (int32 y = 0; y < image.height(); ++y) {
                    image[y][x] = Color{ left.lerp(right, t), image[y][x].a };
                }
            }

            font_sub(TextSub).stampAt(image, region_main.w / 2.0, 107, theme::Palette::DimmedPurple);

            asset.texture = Texture{ image };
            return static_cast<bool>(asset.texture);
        };

        return data;
    }

    void MusicSelect::RegisterAssets() {
        TextureAsset::Register({ Assets::MenuUITitle }, MakeMenuUITitle());
        TextureAsset::Load({ Assets::MenuUITitle });
        components::Tile::RegisterAssets();
    }
}
