#pragma once
#include "Common.hpp"
#include "app/usecases/App.hpp"

using app::App;

namespace ui {
    class Game : public App::Scene {
    public:
        Game(const InitData& init);
        ~Game();

        void update() override;
        void draw() const override;

    private:
        static constexpr double OffsetWait = 0.0;
        double m_tile_offset_raw = -OffsetWait;
        double m_tile_offset = 0.0;
    };
}
