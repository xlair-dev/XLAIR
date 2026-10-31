#pragma once

#include "app/flows/Title.hpp"
#include "ui/Scene.hpp"
#include "ui/components/ErrorPopup.hpp"
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
        void reportState();

        components::TitleBackground m_background;
        components::ErrorPopup m_error_popup;
        Optional<app::card::Error> m_notified_error;
        double m_elapsed = 0.0;
        std::unique_ptr<app::flows::Title> m_title_flow;
    };
}
