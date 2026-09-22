#pragma once

#include "ui/Scene.hpp"

namespace xlair::ui::scenes {
    class ComponentGallery final : public SceneBase {
    public:
        explicit ComponentGallery(const InitData& init);

        void update() override;
        void draw() const override;

    private:
    };
}
