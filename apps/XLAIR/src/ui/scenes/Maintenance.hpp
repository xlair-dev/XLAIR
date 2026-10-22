#pragma once

#include "ui/Scene.hpp"

namespace xlair::ui::scenes {
    class Maintenance final : public SceneBase {
    public:
        explicit Maintenance(const InitData& init);

        void update() override;
        void draw() const override;

    private:
        std::size_t m_selected_index = 0;
        int32 m_opened_frame = Scene::FrameCount();
    };
}
