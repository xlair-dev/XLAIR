#include "TutorialPanel.hpp"

#include "ui/Design.hpp"
#include "ui/assets/Assets.hpp"
#include "ui/theme/Palette.hpp"

#include <cmath>

namespace xlair::ui::components {
    namespace {
        constexpr double CenterX = DesignSize.x / 2.0;

        void DrawText(
            const StringView value,
            const double font_size,
            const double maximum_width,
            const Vec2& center,
            const ColorF& color
        ) {
            const auto text = FontAsset{ assets::font::Text }(value);
            const double size = font_size * Min(1.0, maximum_width / Max(1.0, text.region(font_size).w));
            text.drawAt(size, center, color);
        }
    }

    void DrawTutorialPanel(const TutorialPanelData& data, const double opacity) {
        if (!std::isfinite(opacity) || opacity <= 0.0) {
            return;
        }
        const double alpha = Clamp(opacity, 0.0, 1.0);
        const ColorF text_color = theme::Palette::BlueGray.withA(alpha);
        switch (data.kind) {
            case TutorialPanelKind::Prompt:
                DrawText(data.title, 46.0, 1400.0, Vec2{ CenterX, DesignSize.y / 2.0 }, text_color);
                break;

            case TutorialPanelKind::Explain: {
                const Transformer2D transform{ Mat3x2::Scale(0.96 + 0.04 * alpha, TutorialGuideBounds.center()) *
                                               Mat3x2::Translate(0.0, -16.0 * (1.0 - alpha)) };
                TutorialGuideBounds.draw(ColorF{ 1.0, 0.96 * alpha })
                    .drawFrame(2, 0, theme::Palette::Pink.withA(alpha));
                DrawText(data.title, 38.0, 900.0, Vec2{ CenterX, 165 }, text_color);
                DrawText(data.message, 28.0, 900.0, Vec2{ CenterX, 245 }, text_color);
                break;
            }

            case TutorialPanelKind::Complete:
                DrawText(data.title, 46.0, 1400.0, Vec2{ CenterX, 450 }, text_color);
                DrawText(data.message, 30.0, 1400.0, Vec2{ CenterX, 540 }, text_color);
                break;
        }
    }
}
