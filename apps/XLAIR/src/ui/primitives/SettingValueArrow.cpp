#include "SettingValueArrow.hpp"

#include <algorithm>
#include <array>

namespace xlair::ui::primitives {
    namespace {
        constexpr double Width = 76.0;
        constexpr double Height = 57.0;
        constexpr double HalfHeight = Height / 2.0;

        struct Layer {
            double tip_x;
            double shoulder_x;
            double right_x;
            double fade_start_x;
            double fade_end_x;
            bool secondary_color;
        };

        // Coordinates and gradient stops from the 76 x 57 Figma SVG.
        constexpr std::array<Layer, 4> Layers{ {
            { 0.0, 27.7298, 76.0, 58.6379 * 0.375, 58.6379, false },
            { 13.1538, 40.8836, 71.7414, 13.1538, 68.4655, true },
            { 20.4615, 48.1912, 71.7414, 20.4615, 53.7241, false },
            { 24.8461, 52.5758, 67.1552, 24.8461, 24.8461 + (67.4828 - 24.8461) * 0.605769, true },
        } };

        [[nodiscard]]
        double OpacityAt(const Layer& layer, const double x) {
            if (x <= layer.fade_start_x) {
                return 1.0;
            }
            if (x >= layer.fade_end_x) {
                return 0.0;
            }
            return (layer.fade_end_x - x) / (layer.fade_end_x - layer.fade_start_x);
        }

        [[nodiscard]]
        double TopAt(const Layer& layer, const double x) {
            const double fraction = Clamp((x - layer.tip_x) / (layer.shoulder_x - layer.tip_x), 0.0, 1.0);
            return HalfHeight * (1.0 - fraction);
        }

        void DrawLayer(const Layer& layer, const Vec2& origin, const ColorF& color) {
            // Split at every change in shape or color interpolation. A single Quad across a stop
            // would interpolate its opacity through the wrong interval.
            std::array<double, 5> cuts{
                layer.tip_x, layer.shoulder_x, layer.right_x, layer.fade_start_x, layer.fade_end_x,
            };
            std::sort(cuts.begin(), cuts.end());

            for (std::size_t index = 0; index + 1 < cuts.size(); ++index) {
                const double x0 = cuts[index];
                const double x1 = cuts[index + 1];
                if (x0 == x1 || x0 >= layer.fade_end_x) {
                    continue;
                }

                const double top0 = TopAt(layer, x0);
                const double top1 = TopAt(layer, x1);
                const ColorF color0 = color.withA(color.a * OpacityAt(layer, x0));
                const ColorF color1 = color.withA(color.a * OpacityAt(layer, x1));

                const Vec2 upper0 = origin + Vec2{ x0, top0 };
                const Vec2 upper1 = origin + Vec2{ x1, top1 };
                const Vec2 lower0 = origin + Vec2{ x0, Height - top0 };
                const Vec2 lower1 = origin + Vec2{ x1, Height - top1 };

                if (x0 == layer.tip_x) {
                    Triangle{ upper0, upper1, lower1 }.draw(color0, color1, color1);
                } else {
                    Quad{ upper0, upper1, lower1, lower0 }.draw(color0, color1, color1, color0);
                }
            }
        }
    }

    void DrawSettingValueArrow(
        const Vec2& center,
        const ArrowDirection direction,
        const ColorF& primary,
        const ColorF& secondary,
        const double scale
    ) {
        if (scale <= 0.0) {
            return;
        }

        const Transformer2D transform{
            Mat3x2::Scale(direction == ArrowDirection::Right ? -scale : scale, scale, Float2{ center })
        };
        const Vec2 origin = center - Vec2{ Width / 2.0, HalfHeight };

        for (const auto& layer : Layers) {
            DrawLayer(layer, origin, layer.secondary_color ? secondary : primary);
        }

        LineString{
            origin + Vec2{ 29.0, 4.0 },
            origin + Vec2{ 5.0, HalfHeight },
            origin + Vec2{ 29.0, 53.0 },
        }
            .draw(1.0, Palette::White);
    }
}
