#pragma once

#include "ui/Scene.hpp"
#include "ui/components/GameScoreBar.hpp"
#include "ui/components/SliderMappingGuide.hpp"

namespace xlair::ui::scenes {
    class Result final : public SceneBase {
    public:
        explicit Result(const InitData& init);
        ~Result() override;

        void update() override;
        void updateFadeIn(double t) override;
        void updateFadeOut(double t) override;
        void draw() const override;
        void drawFadeIn(double t) const override;
        void drawFadeOut(double t) const override;

    private:
        void drawResult(const GameResult& result) const;
        void drawFadeOverlay(double opacity) const;

        Optional<GameResult> m_result;
        Array<components::SliderMapping> m_slider_mappings;
        mutable components::GameScoreBar m_score_bar;
        double m_elapsed = 0.0;
        bool m_continue_music = false;
    };
}
