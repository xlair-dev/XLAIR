#pragma once
#include "Common.hpp"
#include "app/usecases/App.hpp"
#include "core/features/DemoMusic.hpp"

#include "ui/theme/Palette.hpp"
#include "ui/components/Tile.hpp"

using app::App;

namespace ui {
    namespace consts {
        static constexpr SizeF SelectedTileSize{ 416, 545 };
        static constexpr SizeF TileSize = 0.88 * SelectedTileSize;
        static constexpr double SelectedTileMargin = 50.0;
        static constexpr double TileSpacing = 50.0;
        static constexpr double TileY = 553.0;
        static constexpr double OffsetWait = 1.0;
    }

    class MusicSelect : public App::Scene {
    public:
        MusicSelect(const InitData& init);
        ~MusicSelect();
        void update() override;
        void draw() const override;

        static void RegisterAssets();

    private:
        components::Tile m_tile;
        double m_tile_offset_raw = -consts::OffsetWait;
        double m_tile_offset = 0.0;

        double m_scroll_offset = 0.0;
        double m_scroll_velocity = 0.0;

        core::features::DemoMusic m_demo;

        struct Assets {
            static constexpr AssetNameView MenuUITitle{ U"MusicSelect.MenuUITitle" };
        };

        void drawBackground() const;
        void drawUI() const;
        void drawTiles() const;
        void drawArrows() const;

        void handleReturnInput();
        void handleIndexInput();
        void handleDifficultyInput();
        void updateScrollState();

        const Array<Color> m_slider_color{
            theme::Palette::Pink,
              theme::Palette::Pink,
            theme::Palette::Pink,
              theme::Palette::Pink,
            theme::Palette::Pink,

              theme::Palette::White,

            theme::Palette::Pink,
              theme::Palette::Pink,
            theme::Palette::Pink,
              theme::Palette::Pink,
            theme::Palette::Pink,

              theme::Palette::White,

            theme::Palette::Cyan,
              theme::Palette::Cyan,
            theme::Palette::Cyan,
              theme::Palette::Cyan,
            theme::Palette::Cyan,
              theme::Palette::Cyan,
            theme::Palette::Cyan,

              theme::Palette::White,

            theme::Palette::Pink,
              theme::Palette::Pink,
            theme::Palette::Pink,

              theme::Palette::White,

            theme::Palette::Pink,
              theme::Palette::Pink,
            theme::Palette::Pink,

              theme::Palette::White,

            theme::Palette::Purple,
              theme::Palette::Purple,
            theme::Palette::Purple,
        };
    };
}
