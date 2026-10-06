#pragma once

#include "ui/Scene.hpp"
#include "ui/Design.hpp"
#include "ui/game/PlayfieldRenderer3D.hpp"
#include <Playfield/ChartProjection.hpp>

#ifndef NDEBUG
namespace xlair::ui::scenes {
    class PlayfieldDebug final : public SceneBase {
    public:
        explicit PlayfieldDebug(const InitData& init);

        void update() override;
        void draw() const override;

    private:
        DebugCamera3D m_camera;
        game::PlayfieldRenderer3D m_renderer;
        sheets::Chart m_chart;
        playfield::ChartProjection m_projection;
        double m_elapsed = 0.0;
        bool m_paused = false;
        bool m_show_depth_guides = false;
    };
}
#endif
