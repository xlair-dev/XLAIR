#pragma once
#include "Common.hpp"
#include "app/usecases/App.hpp"

#include "ui/components/Tile.hpp"

using app::App;

namespace ui {
    class MusicSelect : public App::Scene {
    public:
        MusicSelect(const InitData& init);
        ~MusicSelect();
        void update() override;
        void draw() const override;

        static void RegisterAssets();

    private:
        components::Tile m_tile;

        struct Assets {
            static constexpr AssetNameView MenuUITitle { U"MusicSelect.MenuUITitle" };
        };

        void drawBackground() const;
        void drawUI() const;
        void drawTiles() const;
    };
}
