#include "Tutorial.hpp"

#include <algorithm>
#include <cmath>

namespace xlair::app::flows {
    namespace {
        [[nodiscard]]
        bool AtOrAfterBeat(const std::int64_t sample, const std::int64_t sample_rate, const double beat) noexcept {
            const long double boundary =
                std::round(static_cast<long double>(beat) * sample_rate * 60.0L / Tutorial::BPM);
            return static_cast<long double>(sample) >= boundary;
        }
    }

    void Tutorial::choose(const bool yes) noexcept {
        if (m_state != State::Prompt) {
            return;
        }
        m_state = yes ? State::Playing : State::Skipped;
    }

    void Tutorial::advance(const std::int64_t music_sample, const std::int64_t sample_rate) noexcept {
        if (m_state != State::Playing || sample_rate <= 0) {
            return;
        }
        m_music_sample = std::max<std::int64_t>(0, music_sample);
        m_sample_rate = sample_rate;
        if (AtOrAfterBeat(m_music_sample, m_sample_rate, EndBeat)) {
            m_state = State::Complete;
        }
    }

    Tutorial::State Tutorial::state() const noexcept {
        return m_state;
    }

    Tutorial::Lesson Tutorial::lesson() const noexcept {
        Lesson active = Lesson::Tap;
        for (const auto& guide : Guides) {
            if (m_sample_rate <= 0 || !AtOrAfterBeat(m_music_sample, m_sample_rate, guide.begin_beat)) {
                break;
            }
            active = guide.lesson;
        }
        return active;
    }

    std::optional<Tutorial::Guide> Tutorial::guide() const noexcept {
        if (m_state != State::Playing || m_sample_rate <= 0) {
            return std::nullopt;
        }
        for (const auto& guide : Guides) {
            if (AtOrAfterBeat(m_music_sample, m_sample_rate, guide.begin_beat) &&
                !AtOrAfterBeat(m_music_sample, m_sample_rate, guide.end_beat)) {
                return guide;
            }
        }
        return std::nullopt;
    }
}
