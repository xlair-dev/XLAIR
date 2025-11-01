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
        size_t m_samples = 0;
        int64 m_sample_rate = 44100;
        Array<int64> m_last_judged{ 16 };

        int32 m_late = 0;
        int32 m_fast = 0;
        int32 m_total_diff = 0;
        int32 m_count = 0;
        uint64 m_start_time_ms = 0;

        bool m_started = false;

        const MSRenderTexture m_rt_main_field{ 1000, 4000 };

        void drawField() const;
        void drawMainField() const;
        void drawSideField() const;

        void drawMainNotes() const;

        void drawTouchEffect() const;

        void judgement();

        double calculateNoteY(int64 sample) const;
    };
}
