#include "DifficultyLevel.hpp"

#include "ui/assets/Assets.hpp"
#include "ui/presentation/DifficultyText.hpp"

namespace xlair::ui::components {
    void DrawDifficultyLevel(const double level, const Vec2& baseline, const double max_width, const ColorF& color) {
        constexpr double NumberSize = 87.0;
        constexpr double PlusSize = 35.0;
        constexpr double Gap = 2.0;

        String text = presentation::DifficultyLevelText(level);
        const bool has_plus = text.ends_with(U'+');
        if (has_plus) {
            text.pop_back();
        }
        const Font font = FontAsset{ assets::font::Text };
        const auto number = font(text);
        const auto plus = font(U"+");
        const double width = number.region(NumberSize).w + (has_plus ? Gap + plus.region(PlusSize).w : 0.0);
        const double scale = Clamp(max_width / Max(1.0, width), 0.0, 1.0);
        number.drawBase(NumberSize * scale, baseline, color);
        if (has_plus) {
            plus.drawBase(
                PlusSize * scale,
                baseline.movedBy(number.region(NumberSize * scale).w + Gap * scale, -32.0 * scale),
                color
            );
        }
    }
}
