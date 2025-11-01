#pragma once
#include "Common.hpp"
#include "app/usecases/App.hpp"
#include "app/usecases/Assets.hpp"

using app::App;

namespace ui {
    struct JudgeEffect : IEffect {
        Vec2 position;
        int32 kind;

        JudgeEffect(const Vec2& pos, int32 k) : position(pos), kind(k) {}

        bool update(double t) override {
            const double e = EaseOutExpo(t * 2.0);
            if (kind == 0) {
                // perfect
                TextureAsset(app::assets::texture::Perfect).scaled(0.4).drawAt(position.moveBy(0, -5 * e), ColorF{1.0, 1.0 - Max(0.0, 7.0 * t - 1.0)});
            } else if (kind == 1) {
                // great
                TextureAsset(app::assets::texture::Great).scaled(0.4).drawAt(position.moveBy(0, -5 * e), ColorF{1.0, 1.0 - Max(0.0, 7.0 * t - 1.0)});
            } else if (kind == 2) {
                // good
                TextureAsset(app::assets::texture::Good).scaled(0.4).drawAt(position.moveBy(0, -5 * e), ColorF{1.0, 1.0 - Max(0.0, 7.0 * t - 1.0)});
            } else if (kind == 3) {
                // miss
                TextureAsset(app::assets::texture::Miss).scaled(0.4).drawAt(position.moveBy(0, -5 * e), ColorF{1.0, 1.0 - Max(0.0, 7.0 * t - 1.0)});
            }
            return (t < 0.5);
        }

    };

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

        Effect m_effect;
    };
}
