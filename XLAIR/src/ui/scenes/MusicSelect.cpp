#include "MusicSelect.hpp"

#include "core/types/Difficulty.hpp"
#include "core/features/ControllerManager.hpp"

#include "app/consts/Scene.hpp"
#include "app/usecases/Assets.hpp"

#include "ui/components/Tile.hpp"
#include "ui/components/UserNameplate.hpp"
#include "ui/components/MenuTimerPlate.hpp"
#include "ui/components/SliderMappingGuide.hpp"
#include "ui/primitives/Arrow.hpp"
#include "ui/theme/Palette.hpp"

namespace ui {
    using namespace app::consts;
    using namespace ui::consts;

    MusicSelect::MusicSelect(const InitData& init)
        : IScene(init) {}

    MusicSelect::~MusicSelect() {}

    void MusicSelect::update() {
        handleReturnInput();
        handleIndexInput();
        handleDifficultyInput();
        updateScrollState();

        const auto& index = getData().userData.selected_index;
        const auto& repo = getData().sheetRepository;
        m_demo.update(repo->getMetadata(index).music, repo->getMetadata(index).music_offset);
        m_demo.autoPlayAndStop();
    }

    void MusicSelect::draw() const {

        // drawBackground();

        drawTiles();

        drawArrows();

        drawUI();

        const auto& data = getData();
        components::DrawUserNameplate(data.userData, Point{ 59, 72 });
        components::DrawMenuTimerPlate(Point{ 1599, 72 }, 58, 1);

    }

    void MusicSelect::drawBackground() const {
        const auto& index = getData().userData.selected_index;
        const auto& repo = getData().sheetRepository;
        // TODO: add change animation
        const ScopedCustomShader2D shader{ PixelShaderAsset(app::assets::shader::Grayscale) };
        RectF{ SceneSize.maxComponent() }(*repo->getJacket(index)).draw(ColorF{ 1.0, 1.0, 1.0, 0.05 });
    }

    void MusicSelect::drawUI() const {
        TextureAsset(Assets::MenuUITitle).drawAt(SceneWidth / 2.0, 153);

        // constants
        constexpr double LineWidth = 177;
        constexpr double SideOffset = 397.0;
        constexpr double Y1 = 130.0;
        constexpr double Y2 = 147.0;

        // color stops
        constexpr ColorF C0{ theme::Palette::DimmedPurple, 0.00 };
        constexpr ColorF C1{ theme::Palette::Cyan, 0.94 };
        constexpr ColorF C2{ theme::Palette::Purple, 0.05 };
        constexpr ColorF C3{ theme::Palette::Pink, 0.00 };

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

        // controller
        Array<components::Mapping> mapping{
            {
                components::Mapping::Region{ 0, 3, true, false },
                U"◀ 左",
                theme::Palette::Pink,
            },
            {
                components::Mapping::Region{ 3, 3, false, true },
                U"右 ▶",
                theme::Palette::Pink,
            },
            {
                components::Mapping::Region{ 6, 4 },
                U"選択",
                theme::Palette::Cyan,
            },
            {
                components::Mapping::Region{ 10, 2, true, false },
                U"レベル＋",
                theme::Palette::Pink,
            },
            {
                components::Mapping::Region{ 12, 2, false, true },
                U"レベル－",
                theme::Palette::Pink,
            },
            {
                components::Mapping::Region{ 14, 2 },
                U"設定",
                theme::Palette::Purple,
            },
        };
        components::DrawSliderMappingGuide(mapping);
    }

    void MusicSelect::drawTiles() const {
        const auto& data = getData();
        const auto& index = data.userData.selected_index;
        const auto& difficulty = data.userData.selected_difficulty;
        const auto& repo = data.sheetRepository;

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
            m_tile.get(repo->getMetadata(index), *repo->getJacket(index), difficulty, m_tile_offset)
        ).draw();

        const auto drawSide = [&](int32 dir) {
            const double ds = dir * s;
            const double margin_factor = Math::Min(1.0, 1.0 + ds);
            const double neighbor_scale = Math::Clamp(ds, 0.0, 1.0);

            double x = selected_tile_x + dir * (selected_tile_size.x / 2.0 + TileSpacing + SelectedTileMargin * margin_factor);

            const int32 start = static_cast<int32>(index) + dir;
            const int32 end_cmp = (dir > 0 ? static_cast<int32>(repo->size()) : -1);

            for (int32 i = start; (dir > 0) ? (i < end_cmp) : (i > end_cmp); i += dir) {
                if (x - dir * TileSize.x > SceneWidth or x - dir * TileSize.x < 0) {
                    // out of screen
                    break;
                }

                const Vec2 tile_pos{ x, TileY };
                SizeF tile_size = TileSize;

                if (i == static_cast<int32>(index) + dir) {
                    tile_size = TileSize.lerp(SelectedTileSize, neighbor_scale);
                    x += dir * (TileSpacing + SelectedTileMargin) * neighbor_scale;
                }

                RectF tile{ Arg::center = tile_pos.movedBy(dir * tile_size.x / 2, 0), tile_size };

                tile.drawShadow(Vec2{ 12, 26 }, 32.0, 0, ColorF{ 0, 0, 0, 0.22 });
                tile(
                    m_tile.get(repo->getMetadata(i), *repo->getJacket(i), difficulty)
                ).draw();

                x += dir * (TileSpacing + TileSize.x);
            }
        };

        drawSide(+1);  // right side
        drawSide(-1); // left side
    }

    void MusicSelect::drawArrows() const {
        constexpr ColorF color = theme::Palette::Gray;
        constexpr Vec2 center{ SceneWidth / 2.0, TileY };

        constexpr Vec2 right_center = center.movedBy(SelectedTileSize.x / 2.0 - 10, 0);
        constexpr Vec2 left_center = center.movedBy(-SelectedTileSize.x / 2.0 + 10, 0);

        primitives::DrawArrow(right_center, color, +1);
        primitives::DrawArrow(right_center.movedBy(30, 0), color, +1);

        primitives::DrawArrow(left_center, color, -1);
        primitives::DrawArrow(left_center.movedBy(-30, 0), color, -1);
    }

    void MusicSelect::handleReturnInput() {
        using Controller = core::features::ControllerManager;
        // TODO: refactor (Controller::Slider(12, 8))
        uint32 enter_count = 0;
        for (size_t i = 12; i <= 19; ++i) {
            enter_count += Controller::Slider(i).frameCount();
        }
        bool enter_down = KeyEnter.down() or (enter_count == 1);
        if (enter_down) {
            m_demo.stop(0.3s);
            changeScene(app::types::SceneState::Game);
        }
    }

    void MusicSelect::handleIndexInput() {
        using Controller = core::features::ControllerManager;
        auto& data = getData();
        auto& index = getData().userData.selected_index;
        const auto repo_size = data.sheetRepository->size();

        // TODO: refactor (Controller::Slider(0, 6))
        uint32 left_count_one = 0;
        uint32 left_count = 0;
        for (size_t i = 0; i <= 5; ++i) {
            left_count_one += Controller::Slider(i).frameCount() == 1;
            left_count += Controller::Slider(i).frameCount() > 1;
        }
        bool left_down = KeyLeft.down() or (left_count_one > 0 and left_count == 0);

        // TODO: refactor (Controller::Slider(6, 6))
        uint32 right_count = 0;
        for (size_t i = 6; i <= 11; ++i) {
            right_count += Controller::Slider(i).frameCount();
        }
        bool right_down = KeyRight.down() or right_count == 1;

        if (index > 0 and left_down) {
            --index;
            m_scroll_offset = 1.0;
            m_tile_offset_raw = -OffsetWait;
        }
        if (index + 1 < repo_size and right_down) {
            ++index;
            m_scroll_offset = -1.0;
            m_tile_offset_raw = -OffsetWait;
        }
    }

    void MusicSelect::handleDifficultyInput() {
        using Controller = core::features::ControllerManager;
        auto& data = getData();
        auto& difficulty = data.userData.selected_difficulty;

        // TODO: refactor (Controller::Slider(20, 4))
        bool down_down = KeyDown.down();
        for (size_t i = 20; i <= 23; ++i) {
            down_down |= Controller::Slider(i).down();
        }
        // TODO: refactor (Controller::Slider(24, 4))
        bool up_down = KeyUp.down();
        for (size_t i = 24; i <= 27; ++i) {
            up_down |= Controller::Slider(i).down();
        }

        if (difficulty > 0 and down_down) {
            --difficulty;
        }
        if (difficulty + 1 < core::types::DifficultySize and up_down) {
            ++difficulty;
        }
    }

    void MusicSelect::updateScrollState() {
        m_scroll_offset = Math::SmoothDamp(m_scroll_offset, 0.0, m_scroll_velocity, 0.1);
        m_tile_offset_raw += Scene::DeltaTime();
        m_tile_offset = Math::Max(0.0, m_tile_offset_raw);
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
