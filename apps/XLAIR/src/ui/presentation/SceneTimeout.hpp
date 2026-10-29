#pragma once

#include <algorithm>
#include <cmath>

namespace xlair::ui::presentation {
    class SceneTimeout {
    public:
        explicit SceneTimeout(const double seconds)
            : m_duration{ std::isfinite(seconds) ? std::max(0.0, seconds) : 0.0 } {}

        [[nodiscard]]
        bool update(const bool active, const double delta_seconds) noexcept {
            if (!active) {
                m_elapsed = 0.0;
                m_active = false;
                return false;
            }
            // The activation frame may include time spent in a different state or a scene transition.
            if (m_active && std::isfinite(delta_seconds) && delta_seconds > 0.0) {
                m_elapsed = std::min(m_duration, m_elapsed + delta_seconds);
            }
            m_active = true;
            return m_elapsed >= m_duration;
        }

        [[nodiscard]]
        double remainingSeconds() const noexcept {
            return m_duration - m_elapsed;
        }

    private:
        double m_duration = 0.0;
        double m_elapsed = 0.0;
        bool m_active = false;
    };
}
