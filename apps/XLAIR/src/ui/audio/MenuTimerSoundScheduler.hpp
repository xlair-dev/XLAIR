#pragma once

#include <cmath>

namespace xlair::ui::audio {
    class MenuTimerSoundScheduler {
    public:
        enum class Kind { None, Normal, Special };

        struct Cue {
            Kind kind = Kind::None;
            double offset_seconds = 0.0;
        };

        [[nodiscard]]
        Cue update(const double remaining_seconds) noexcept {
            if (!std::isfinite(remaining_seconds) || remaining_seconds <= 0.0) {
                return {};
            }

            const double count = std::ceil(remaining_seconds);
            const bool changed = count != m_previous_count;
            m_previous_count = count;
            if (count > 20.0) {
                return {};
            }
            if (count > 3.0) {
                return { changed ? Kind::Normal : Kind::None };
            }
            if (m_special_started) {
                return {};
            }
            m_special_started = true;
            return { Kind::Special, 3.0 - remaining_seconds };
        }

        void reset() noexcept {
            m_previous_count = -1.0;
            m_special_started = false;
        }

    private:
        double m_previous_count = -1.0;
        bool m_special_started = false;
    };
}
