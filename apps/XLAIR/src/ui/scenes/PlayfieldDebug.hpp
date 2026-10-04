#pragma once

#include "ui/Scene.hpp"
#include "ui/Design.hpp"

#ifndef NDEBUG
namespace xlair::ui::scenes {
    class PlayfieldDebug final : public SceneBase {
    public:
        explicit PlayfieldDebug(const InitData& init);

        void update() override;
        void draw() const override;

    private:
        DebugCamera3D m_camera;
        MSRenderTexture m_render_texture;
        double m_elapsed = 0.0;
    };
}
#endif
