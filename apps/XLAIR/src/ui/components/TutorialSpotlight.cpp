#include "TutorialSpotlight.hpp"

#include "ui/Design.hpp"

#include <cmath>

namespace xlair::ui::components {
    TutorialSpotlight::TutorialSpotlight() : m_texture{ DesignSize, ColorF{ 0.0, 0.0 } } {
        if (!m_texture) {
            throw Error{ U"Failed to create the tutorial spotlight texture." };
        }
    }

    void TutorialSpotlight::draw(const std::span<const Quad> regions, const double opacity) const {
        if (!std::isfinite(opacity) || opacity <= 0.0) {
            return;
        }

        const Transformer2D local{ Mat3x2::Identity(), Transformer2D::Target::SetLocal };
        const Transformer2D camera{ Mat3x2::Identity(), Transformer2D::Target::SetCamera };
        const ScopedViewport2D viewport{ DesignSize };
        const ScopedColorMul2D color_mul{ 1.0 };
        const ScopedColorAdd2D color_add{ 0.0 };
        const ScopedRenderStates2D render_states{
            BlendState::NonPremultiplied,
            RasterizerState::SolidCullNone,
            SamplerState::ClampNearest,
        };
        {
            const ScopedRenderTarget2D target{ m_texture.clear(ColorF{ 0.20, 0.58 * Clamp(opacity, 0.0, 1.0) }) };
            const ScopedRenderStates2D overwrite{ BlendState::Opaque };
            for (const auto& region : regions) {
                region.draw(ColorF{ 0.0, 0.0 });
            }
        }
        Graphics2D::Flush();
        m_texture.draw();
    }
}
