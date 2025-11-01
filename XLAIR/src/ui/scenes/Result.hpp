#pragma once
#include "Common.hpp"
#include "app/usecases/App.hpp"

using app::App;

namespace ui {
    class Result : public App::Scene {
    public:
        Result(const InitData& init);
        ~Result();

        void update() override;
        void draw() const override;

    private:
        static constexpr double OffsetWait = 0.0;
        double m_tile_offset = 0.0;
        double m_scene_timer = 0.0;
        int32 m_rating = 0;

        bool m_update_record = false;
    };
}
