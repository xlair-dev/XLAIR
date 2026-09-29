#pragma once

#include "ui/Scene.hpp"

namespace xlair::ui::scenes {
    class Settings final : public SceneBase {
    public:
        explicit Settings(const InitData& init);

        void update() override;
        void draw() const override;

    private:
    };
}
