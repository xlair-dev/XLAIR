#pragma once
#include "Common.hpp"
#include "app/usecases/App.hpp"

#include "ui/components/Tile.hpp"

using app::App;

namespace ui {
    namespace consts {
        static constexpr SizeF SelectedTileSize{ 416, 545 };
        static constexpr SizeF TileSize = 0.88 * SelectedTileSize;
        static constexpr double SelectedTileMargin = 50.0;
        static constexpr double TileSpacing = 50.0;
        static constexpr double TileY = 553.0;
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

        size_t m_selected_index = 0;
        double m_scroll_offset = 0.0;
        double m_scroll_velocity = 0.0;

        struct Assets {
            static constexpr AssetNameView MenuUITitle{ U"MusicSelect.MenuUITitle" };
        };

        void drawBackground() const;
        void drawUI() const;
        void drawTiles() const;
    };
}
