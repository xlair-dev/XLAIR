#include "TitleBackground.hpp"

#include "ui/Design.hpp"

#include <array>
#include <cmath>

namespace xlair::ui::components {
    namespace {
        struct GlowOrbit {
            Vec2 origin;
            double radius;
            double size;
            double period;
            double phase;
            ColorF color;
        };

        Image MakeGlowImage() {
            return Image{ 256, Arg::generator0_1 = [](const Vec2 uv) {
                             const double radius_squared = (uv - Vec2{ 0.5, 0.5 }).lengthSq() * 4.0;
                             const double alpha =
                                 std::exp(-radius_squared * 4.0) * Clamp((1.0 - radius_squared) * 4.0, 0.0, 1.0);
                             return ColorF{ 1.0, alpha }.toColor();
                         } };
        }
    }

    TitleBackground::TitleBackground()
        : m_pattern{ Resource(U"ui/textures/title-pattern.png"), TextureDesc::Mipped },
          m_strip{ Resource(U"ui/textures/title-strip.png"), TextureDesc::Mipped },
          m_glow{ MakeGlowImage(), TextureDesc::Mipped } {
        if (!m_pattern || !m_strip || !m_glow) {
            throw Error{ U"Failed to load the title background textures." };
        }
    }

    void TitleBackground::draw(const double elapsed_seconds) const {
        Scene::Rect().draw(ColorF{ 1.0 });

        const std::array orbits{
            GlowOrbit{ .origin = { 1.02, 0.48 },
                       .radius = 150.0,
                       .size = 1300.0,
                       .period = 10.0,
                       .phase = 20_deg,
                       .color = ColorF{ U"#18ACE9" }.withA(0.22) },
            GlowOrbit{ .origin = { 0.02, 0.9 },
                       .radius = 115.0,
                       .size = 1100.0,
                       .period = 7.0,
                       .phase = 40_deg,
                       .color = ColorF{ U"#F495EA" }.withA(0.22) },
            GlowOrbit{ .origin = { 0.9, 1.02 },
                       .radius = 115.0,
                       .size = 1100.0,
                       .period = -7.0,
                       .phase = -30_deg,
                       .color = ColorF{ U"#F495EA" }.withA(0.18) },
            GlowOrbit{ .origin = { -0.05, 0.35 },
                       .radius = 150.0,
                       .size = 1300.0,
                       .period = -10.0,
                       .phase = -10_deg,
                       .color = ColorF{ U"#3AD0F5" }.withA(0.18) },
        };
        for (const auto& orbit : orbits) {
            const double angle = elapsed_seconds * Math::TwoPi / orbit.period + orbit.phase;
            const Vec2 origin{ orbit.origin.x * DesignSize.x, orbit.origin.y * DesignSize.y };
            const Vec2 center = origin + Vec2{ Math::Cos(angle), Math::Sin(angle) } * orbit.radius;
            m_glow.resized(orbit.size).drawAt(center, orbit.color);
        }

        constexpr double StripHeight = 320.0;
        constexpr double StripPeriod = 18.0;
        const double strip_width = StripHeight * m_strip.width() / m_strip.height();
        const double offset = std::fmod(elapsed_seconds, StripPeriod) / StripPeriod * strip_width;
        for (double x = -offset; x < DesignSize.x; x += strip_width) {
            m_strip.resized(strip_width, StripHeight).draw(x, 0);
        }

        const double pattern_height = DesignSize.y;
        const double pattern_width = pattern_height * m_pattern.width() / m_pattern.height();
        constexpr double FadeFraction = 0.3;
        constexpr double PatternLeft = -80.0;
        m_pattern.uv(0, 0, 1, FadeFraction)
            .resized(pattern_width, pattern_height * FadeFraction)
            .draw(PatternLeft, 0, Arg::top = ColorF{ 1.0, 0.0 }, Arg::bottom = ColorF{ 1.0, 0.8 });
        m_pattern.uv(0, FadeFraction, 1, 1.0 - FadeFraction)
            .resized(pattern_width, pattern_height * (1.0 - FadeFraction))
            .draw(PatternLeft, pattern_height * FadeFraction, ColorF{ 1.0, 0.8 });
    }
}
