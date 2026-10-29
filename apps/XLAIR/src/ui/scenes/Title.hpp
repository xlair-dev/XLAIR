#pragma once

#include "app/flows/Title.hpp"
#include "ui/Scene.hpp"
#include "ui/components/TitleBackground.hpp"

namespace xlair::ui::scenes {
    class Title final : public SceneBase {
    public:
        explicit Title(const InitData& init);

        void update() override;
        void draw() const override;
        void updateFadeOut(double t) override;
        void drawFadeOut(double t) const override;

    private:
        void reportState() const;

        components::TitleBackground m_background;
        double m_elapsed = 0.0;
        std::unique_ptr<app::flows::Title> m_title_flow;
    };
}
