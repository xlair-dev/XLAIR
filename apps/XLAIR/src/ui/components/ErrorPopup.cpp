#include "ErrorPopup.hpp"

#include "ui/assets/Assets.hpp"
#include "ui/theme/Palette.hpp"

#include <cmath>
#include <utility>

namespace xlair::ui::components {
    void ErrorPopup::show(String title, String message, const double duration_seconds) {
        m_title = std::move(title);
        m_message = std::move(message);
        m_elapsed = 0.0;
        m_duration = std::isfinite(duration_seconds) ? Max(0.0, duration_seconds) : 0.0;
    }

    void ErrorPopup::update(const double delta_seconds) {
        if (std::isfinite(delta_seconds) && delta_seconds > 0.0) {
            m_elapsed = Min(m_duration, m_elapsed + delta_seconds);
        }
    }

    bool ErrorPopup::visible() const noexcept {
        return m_elapsed < m_duration;
    }

    void ErrorPopup::draw(const Vec2& center) const {
        if (!visible()) {
            return;
        }
        const double enter = EaseOutCubic(Clamp(m_elapsed / 0.22, 0.0, 1.0));
        const double leave = Clamp((m_duration - m_elapsed) / 0.25, 0.0, 1.0);
        const double opacity = enter * leave;
        Scene::Rect().draw(ColorF{ 0.0, 0.12 * opacity });

        const double scale = 0.96 + 0.04 * enter;
        const Transformer2D transform{ Mat3x2::Scale(scale, center) * Mat3x2::Translate(0.0, 18.0 * (1.0 - enter)) };
        const RectF panel{ Arg::center = center, 900, 180 };
        panel.drawShadow(Vec2{ 0, 8 }, 24, 0, ColorF{ 0.0, 0.15 * opacity });
        panel.draw(ColorF{ 1.0, opacity }).drawFrame(2, 0, theme::Palette::Pink.withA(opacity));
        RectF{ panel.x, panel.y, 6, panel.h }.draw(theme::Palette::Pink.withA(opacity));
        Triangle{ panel.tl(), panel.tl().movedBy(26, 0), panel.tl().movedBy(0, 26) }.draw(
            theme::Palette::Pink.withA(opacity)
        );
        FontAsset{ assets::font::Text }(m_title)
            .drawAt(36, center.movedBy(0, -28), theme::Palette::Gray.withA(opacity));
        FontAsset{ assets::font::Text }(m_message)
            .drawAt(26, center.movedBy(0, 32), theme::Palette::Gray.withA(opacity));
    }
}
