#pragma once

#include "app/flows/FinalResult.hpp"
#include "ui/Scene.hpp"
#include "ui/input/MenuInput.hpp"

namespace xlair::ui::scenes {
    class FinalResult final : public SceneBase {
    public:
        explicit FinalResult(const InitData& init);
        ~FinalResult() override;

        void update() override;
        void updateFadeIn(double t) override;
        void draw() const override;
        void drawFadeIn(double t) const override;
        void drawFadeOut(double t) const override;

    private:
        void updateSave();

        app::flows::FinalResult m_save_flow;
        input::MenuInput m_menu_input;
        double m_elapsed = 0.0;
        bool m_return_requested = false;
    };
}
