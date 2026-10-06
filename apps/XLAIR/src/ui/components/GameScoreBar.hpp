#pragma once

#include "Common.hpp"
#include "ui/Design.hpp"

namespace xlair::ui::components {
    struct GameScoreBarData {
        uint32 score = 0;
        double clear_gauge = 0.0;
    };

    class GameScoreBar {
    public:
        void draw(const GameScoreBarData& data, const Point& position, int32 width = 868);

    private:
        void updateScoreShadow(StringView score_text, const Font& font, int32 width);

        RenderTexture m_shadow_texture{ DesignSize };
        RenderTexture m_gaussian_a4{ DesignSize / 4 };
        RenderTexture m_gaussian_b4{ DesignSize / 4 };
        Optional<uint32> m_cached_score;
        int32 m_cached_width = 0;
    };
}
