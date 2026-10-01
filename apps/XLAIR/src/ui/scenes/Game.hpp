#pragma once

#include "ui/Scene.hpp"

namespace xlair::ui::scenes {
    class Game final : public SceneBase {
    public:
        explicit Game(const InitData& init);

        void update() override;
        void draw() const override;

    private:
    };
}
