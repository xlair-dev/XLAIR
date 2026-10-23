#pragma once

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

    private:
        Array<components::SliderMapping> m_scroll_mappings;
        std::size_t m_first_record = 0;
        double m_elapsed = 0.0;
    };
}
