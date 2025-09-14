#include "MusicSelect.hpp"

#include "app/consts/Scene.hpp"

#include "ui/components/Tile.hpp"
#include "ui/components/PlayerNameplate.hpp"
#include "ui/components/MenuTimerPlate.hpp"

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
            }
        }
        if (m_selected_index > 0) {
            if (KeyLeft.down()) {
                --m_selected_index;
                m_scroll_offset = 1.0;
            }
        }

        m_scroll_offset = Math::SmoothDamp(m_scroll_offset, 0.0, m_scroll_velocity, 1);
    }

    void MusicSelect::draw() const {
        // drawBackground();

        drawUI();

        drawTiles();

        components::DrawPlayerNameplate(Point{ 59, 72 });
        components::DrawMenuTimerPlate(Point{ 1599, 72 }, 58, 1);

        /*
        RectF { TileSize }(
            m_tile.get(getData().sheetRepository.getMetadata(0), *getData().sheetRepository.getJacket(0), 0)
        ).draw(100, 300);
        */
    }

    void MusicSelect::drawBackground() const {
        // TODO: use selected jecket
        const ScopedCustomShader2D shader{ PixelShaderAsset(U"grayscale") };
        RectF{ SceneSize.maxComponent() }(*getData().sheetRepository.getJacket(0)).draw(ColorF{ 1.0, 1.0, 1.0, 0.05 });
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
        constexpr ColorF C3{ ColorF{ U"#FFA1FF" }, 0.00 };

        static const auto drawBar = [&](double base, double dir) {
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

        const Vec2 center{ SceneWidth / 2.0, TileY };

        // scroll value (right:+ / left:-)
        const double s = m_scroll_offset;
        const double s_abs = Math::Abs(s);
        const double s_right = Math::Clamp(s, 0.0, 1.0);
        const double s_left = Math::Clamp(-s, 0.0, 1.0);

        // selected tile
        const SizeF selected_tile_size = SelectedTileSize.lerp(TileSize, s_abs);
        const double neighbor_gap = (SelectedTileSize.x / 2.0 + TileSize.x / 2.0 + TileSpacing + SelectedTileMargin);
        const double selected_tile_x = center.x - neighbor_gap * s;

        const RectF selected_tile{ Arg::center = Vec2{ selected_tile_x, TileY }, selected_tile_size };
        selected_tile.draw(Palette::Black);

        static const auto drawSide = [&](int32 dir) {
            const double margin_factor = Math::Min(1.0, 1.0 + dir * s);
            
            double x = selected_tile_x + dir * (selected_tile_size.x / 2.0 + TileSpacing + SelectedTileMargin * margin_factor);
            const double neighbor_scale = (dir > 0 ? s_right : s_left);

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
                tile.draw(Palette::Gray);

                x += dir * (TileSpacing + TileSize.x);
            }
        };

        drawSide(+1);  // right side
        drawSide(-1); // left side
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
                // TODO: define colors somewhere
                constexpr ColorF left = ColorF{ U"#FF8EFF" };
                constexpr ColorF right = ColorF{ U"#35BEFF" };
                for (int32 y = 0; y < image.height(); ++y) {
                    image[y][x] = Color{ left.lerp(right, t), image[y][x].a };
                }
            }

            // TODO: define colors somewhere
            constexpr ColorF SubColor = ColorF{ U"#9B9FCF" };
            font_sub(TextSub).stampAt(image, region_main.w / 2, 107, SubColor);

            asset.texture = Texture{ image };
            return static_cast<bool>(asset.texture);
            };

        return data;
    }

    void MusicSelect::RegisterAssets() {
        TextureAsset::Register({ Assets::MenuUITitle }, MakeMenuUITitle());
        TextureAsset::Load({ Assets::MenuUITitle });
    }
}
