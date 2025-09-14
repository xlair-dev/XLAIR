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

        m_scroll_offset = Math::SmoothDamp(m_scroll_offset, 0.0, m_scroll_velocity, 0.1);
    }

    void MusicSelect::draw() const {
        Size TileSize{ 416, 505 };

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
        const auto center = Vec2{ SceneWidth / 2.0, TileY };

        // selected tile
        const auto selected_tile_size = SelectedTileSize.lerp(TileSize, Math::Abs(m_scroll_offset));
        const auto selected_tile_x = center.x - (SelectedTileSize.x / 2.0 + TileSize.x / 2.0 + TileSpacing + SelectedTileMargin) * m_scroll_offset;
        RectF selected_tile{ Arg::center = Vec2{ selected_tile_x, TileY }, selected_tile_size };
        selected_tile.draw(Palette::Black);

        double x = selected_tile_x + (selected_tile_size.x / 2.0 + TileSpacing + SelectedTileMargin * Math::Min(1.0, 1.0 + m_scroll_offset));
        // right side
        for (int32 i = static_cast<int32>(m_selected_index) + 1; i < repo.size(); ++i) {
            if (x - TileSize.x > SceneWidth) {
                // out of screen
                break;
            }

            RectF tile{ Arg::leftCenter = Vec2{ x, TileY }, TileSize };

            if (i == static_cast<int32>(m_selected_index) + 1) {
                tile.set(Arg::leftCenter = Vec2{ x, TileY }, TileSize.lerp(SelectedTileSize, Math::Max(0.0, m_scroll_offset)));
                x += (TileSpacing + SelectedTileMargin) * Math::Max(0.0, m_scroll_offset);
            }

            tile.draw(Palette::Gray);

            x += TileSpacing + TileSize.x;
        }

        x = selected_tile_x - (selected_tile_size.x / 2.0 + TileSpacing + SelectedTileMargin * Math::Min(1.0, 1.0 - m_scroll_offset));
        // left side
        for (int32 i = static_cast<int32>(m_selected_index) - 1; i >= 0; --i) {
            if (x + TileSize.x > SceneWidth) {
                // out of screen
                break;
            }

            RectF tile{ Arg::rightCenter = Vec2{ x, TileY }, TileSize };

            if (i == static_cast<int32>(m_selected_index) - 1) {
                tile.set(Arg::rightCenter = Vec2{ x, TileY }, TileSize.lerp(SelectedTileSize, Math::Max(0.0, -m_scroll_offset)));
                x -= (TileSpacing + SelectedTileMargin) * Math::Max(0.0, -m_scroll_offset);
            }

            tile.draw(Palette::Gray);

            x -= TileSpacing + TileSize.x;
        }
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
