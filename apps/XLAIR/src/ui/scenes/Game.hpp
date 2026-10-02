#pragma once

#include "ui/Scene.hpp"

namespace xlair::ui::scenes {
    class Game final : public SceneBase {
    public:
        explicit Game(const InitData& init);
        ~Game() override;

        void update() override;
        void draw() const override;
        void drawFadeIn(double t) const override;
        void drawFadeOut(double t) const override;

    private:
        void drawFadeOverlay(double opacity) const;
    };
}
