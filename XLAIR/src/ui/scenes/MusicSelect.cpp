#include "MusicSelect.hpp"

#include "app/consts/Scene.hpp"

#include "ui/components/Tile.hpp"
#include "ui/components/PlayerNameplate.hpp"
#include "ui/components/MenuTimerPlate.hpp"

namespace ui {
    using namespace app::consts;

    MusicSelect::MusicSelect(const InitData& init) : IScene(init) {}

    MusicSelect::~MusicSelect() {}

    void MusicSelect::update() {}

    void MusicSelect::draw() const {
        Size TileSize { 416, 505 };

        drawUI();

        // drawBackground();

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
        const ScopedCustomShader2D shader { PixelShaderAsset(U"grayscale") };
        RectF { SceneSize.maxComponent() }(*getData().sheetRepository.getJacket(0)).draw(ColorF{ 1.0, 1.0, 1.0, 0.05 });
    }

    void MusicSelect::drawUI() const {
        TextureAsset(Assets::MenuUITitle).drawAt(SceneWidth / 2.0, 153);

        constexpr double LineWidth = 177;
        // left
        {
            constexpr double T0 = 397 + LineWidth;
            constexpr double T1 = T0 - LineWidth * 0.06;
            constexpr double T2 = T0 - LineWidth * 0.86;
            constexpr double T3 = T0 - LineWidth;

            constexpr ColorF C0{ U"#9B9FCF" };
            constexpr ColorF C1{ U"#79C8F3" };
            constexpr ColorF C2{ U"#EFEEFE" };
            constexpr ColorF C3{ U"#FFA1FF" };

            Line { T0, 130, T1, 130 }.draw(3, ColorF { C0, 0.00 }, ColorF { C1, 0.94 });
            Line { T0, 147, T1, 147 }.draw(3, ColorF { C0, 0.00 }, ColorF { C1, 0.94 });

            Line { T1, 130, T2, 130 }.draw(3, ColorF { C1, 0.94 }, ColorF{ C2 });
            Line { T1, 147, T2, 147 }.draw(3, ColorF { C1, 0.94 }, ColorF{ C2 });

            Line { T2, 130, T3, 130 }.draw(3, ColorF { C2 }, ColorF{ C3, 0.00 });
            Line { T2, 147, T3, 147 }.draw(3, ColorF { C2 }, ColorF{ C3, 0.00 });
        }

        // right
        {
            constexpr double T0 = SceneWidth - 397 - LineWidth;
            constexpr double T1 = T0 + LineWidth * 0.06;
            constexpr double T2 = T0 + LineWidth * 0.86;
            constexpr double T3 = T0 + LineWidth;

            constexpr ColorF C0{ U"#9B9FCF" };
            constexpr ColorF C1{ U"#79C8F3" };
            constexpr ColorF C2{ U"#EFEEFE" };
            constexpr ColorF C3{ U"#FFA1FF" };

            Line { T0, 130, T1, 130 }.draw(3, ColorF { C0, 0.00 }, ColorF { C1, 0.94 });
            Line { T0, 147, T1, 147 }.draw(3, ColorF { C0, 0.00 }, ColorF { C1, 0.94 });

            Line { T1, 130, T2, 130 }.draw(3, ColorF { C1, 0.94 }, ColorF{ C2 });
            Line { T1, 147, T2, 147 }.draw(3, ColorF { C1, 0.94 }, ColorF{ C2 });

            Line { T2, 130, T3, 130 }.draw(3, ColorF { C2 }, ColorF{ C3, 0.00 });
            Line { T2, 147, T3, 147 }.draw(3, ColorF { C2 }, ColorF{ C3, 0.00 });
        }
    }

    void MusicSelect::drawTiles() const {
        const auto& repo = getData().sheetRepository;

        // selected
        RectF { Arg::center = Vec2{ SceneWidth / 2, 553 }, 416, 505 }.draw(Palette::Black);

        RectF { Arg::center = Vec2{ SceneWidth / 2 + 416 / 2.0 + 100 + 369 / 2.0, 553 }, 369, 450 }.draw(Palette::Black);
        RectF { Arg::center = Vec2{ SceneWidth / 2 - 416 / 2.0 - 100 - 369 / 2.0, 553 }, 369, 450 }.draw(Palette::Black);

        RectF { Arg::center = Vec2{ SceneWidth / 2 + 416 / 2.0 + 100 + 369 / 2.0 + 369 / 2.0 + 50 + 369 / 2.0, 553 }, 369, 450 }.draw(Palette::Black);
        RectF { Arg::center = Vec2{ SceneWidth / 2 - 416 / 2.0 - 100 - 369 / 2.0 - 369 / 2.0 - 50 - 369 / 2.0, 553 }, 369, 450 }.draw(Palette::Black);
    }

    std::unique_ptr<TextureAssetData> MakeMenuUITitle() {
        std::unique_ptr<TextureAssetData> data = std::make_unique<TextureAssetData>();

        data->onLoad = [](TextureAssetData& asset, const String&) {
            constexpr StringView TextMain = U"MUSIC SELECT";
            constexpr StringView TextSub = U"楽曲を選択してください";

            Font font_main{ 70, U"assets/fonts/BrunoAce/BrunoAce-Regular.ttf" };
            const auto region_main = font_main(TextMain).region(70).asRect();

            Font font_sub { 24, Typeface::CJK_Regular_JP };
            const auto region_sub = font_sub(TextSub).region(24).asRect();

            Image image { static_cast<size_t>(region_main.w), static_cast<size_t>(region_main.h + region_sub.h) };
            font_main(TextMain).stamp(image, 0, 0);

            for (int32 x = 0; x < image.width(); ++x) {
                const double t = static_cast<double>(x) / image.width();
                // TODO: define colors somewhere
                constexpr ColorF left = ColorF { U"#FF8EFF" };
                constexpr ColorF right = ColorF { U"#35BEFF" };
                for (int32 y = 0; y < image.height(); ++y) {
                    image[y][x] = Color{ left.lerp(right, t), image[y][x].a };
                }
            }

            // TODO: define colors somewhere
            constexpr ColorF SubColor = ColorF { U"#9B9FCF" };
            font_sub(TextSub).stampAt(image, region_main.w / 2, 107, SubColor);

            asset.texture = Texture { image };
            return static_cast<bool>(asset.texture);
        };

        return data;
    }

    void MusicSelect::RegisterAssets() {
        TextureAsset::Register({ Assets::MenuUITitle }, MakeMenuUITitle());
        TextureAsset::Load({ Assets::MenuUITitle });
    }
}
