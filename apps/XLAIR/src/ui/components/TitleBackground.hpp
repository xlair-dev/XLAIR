#pragma once

#include "Common.hpp"

namespace xlair::ui::components {
    class TitleBackground {
    public:
        TitleBackground();

        void draw(double elapsed_seconds) const;

    private:
        Texture m_pattern;
        Texture m_strip;
        Texture m_glow;
    };
}
