#pragma once

#include "app/flows/FinalResult.hpp"
#include "ui/Scene.hpp"
#include "ui/components/SliderMappingGuide.hpp"

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
        struct ResultRow {
            String sheet_label;
            String score_label;
            String clear_label;
        };

        void updateSave();

        app::flows::FinalResult m_save_flow;
        Array<ResultRow> m_rows;
        std::size_t m_first_record = 0;
        double m_elapsed = 0.0;
    };
}
