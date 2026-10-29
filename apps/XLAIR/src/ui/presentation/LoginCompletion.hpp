#pragma once

#include <algorithm>
#include <cmath>

namespace xlair::ui::presentation {
    class LoginCompletion {
    public:
        enum class Action { None, Announce, Continue };

        explicit LoginCompletion(const double sound_seconds)
            : m_wait_seconds{ std::isfinite(sound_seconds) ? std::max(1.0, sound_seconds) : 1.0 } {}

        [[nodiscard]]
        Action update(const bool ready, const double delta_seconds) {
            const double delta = std::isfinite(delta_seconds) && delta_seconds > 0.0 ? delta_seconds : 0.0;
            m_elapsed += delta;
            if (!ready || m_finished) {
                return Action::None;
            }
            if (!m_announced) {
                if (m_elapsed < 0.5) {
                    return Action::None;
                }
                m_announced = true;
                return Action::Announce;
            }
            m_completion_elapsed += delta;
            if (m_completion_elapsed < m_wait_seconds) {
                return Action::None;
            }
            m_finished = true;
            return Action::Continue;
        }

        [[nodiscard]]
        bool announced() const noexcept {
            return m_announced;
        }

    private:
        double m_wait_seconds = 1.0;
        double m_elapsed = 0.0;
        double m_completion_elapsed = 0.0;
        bool m_announced = false;
        bool m_finished = false;
    };
}
