#pragma once

#include "Common.hpp"

namespace xlair::ui::components {
    struct GameScoreBarData {
        uint32 score = 0;
        double clear_gauge = 0.0;
    };

    class GameScoreBar {
    public:
        void draw(const GameScoreBarData& data, const Point& position, int32 width = 868);

    private:
        static constexpr Size ShadowSize{ 512, 128 };

        void updateScoreShadow(StringView score_text, const Font& font, int32 width);

        RenderTexture m_shadow_texture{ ShadowSize };
        RenderTexture m_gaussian_a4{ ShadowSize / 4 };
        RenderTexture m_gaussian_b4{ ShadowSize / 4 };
        int32 m_shadow_left = 0;
        Optional<uint32> m_cached_score;
        int32 m_cached_width = 0;
    };
}
