#pragma once

#include "ui/Scene.hpp"

namespace xlair::ui::scenes {
    class MusicSelect final : public SceneBase {
    public:
        explicit MusicSelect(const InitData& init);

        void update() override;
        void draw() const override;

    private:
    };
}
