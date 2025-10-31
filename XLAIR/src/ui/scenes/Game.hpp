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

        static void RegisterAssets();

    private:
        static constexpr double OffsetWait = 0.0;
        static constexpr double JudgeLineY = 100.0;
        double m_tile_offset = 0.0;
        double m_scene_timer = 0.0;
        int64 m_pos_sample = 0;
        bool m_started = false;

        const MSRenderTexture m_rt_main_field{ 1000, 4000 };

        void drawField() const;
        void drawMainField() const;
        void drawSideField() const;

        void drawMainNotes() const;

        double calculateNoteY(int64 sample) const;
    };
}
