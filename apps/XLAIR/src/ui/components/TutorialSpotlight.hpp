#pragma once

#include "Common.hpp"

#include <span>

namespace xlair::ui::components {
    class TutorialSpotlight {
    public:
        TutorialSpotlight();

        void draw(std::span<const Quad> regions, double opacity) const;

    private:
        RenderTexture m_texture;
    };
}
