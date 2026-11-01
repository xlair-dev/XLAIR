#pragma once

#include <array>
#include <cstdint>
#include <optional>

namespace xlair::app::flows {
    class Tutorial {
    public:
        enum class State {
            Prompt,
            Playing,
            Complete,
            Skipped,
        };

        enum class Lesson {
            Tap,
            Flick,
            Hold,
            SideUpper,
            SideLower,
        };

        struct Guide {
            double begin_beat;
            double end_beat;
            Lesson lesson;
        };

        static constexpr double BPM = 165.0;
        static constexpr double EndBeat = 164.0;
        static constexpr std::array<Guide, 5> Guides{
            Guide{ 8.0, 20.0, Lesson::Tap },          Guide{ 36.0, 48.0, Lesson::Flick },
            Guide{ 60.0, 72.0, Lesson::Hold },        Guide{ 96.0, 108.0, Lesson::SideUpper },
            Guide{ 128.0, 140.0, Lesson::SideLower },
        };

        void choose(bool yes) noexcept;
        void advance(std::int64_t music_sample, std::int64_t sample_rate) noexcept;

        [[nodiscard]]
        State state() const noexcept;

        [[nodiscard]]
        Lesson lesson() const noexcept;

        [[nodiscard]]
        std::optional<Guide> guide() const noexcept;

    private:
        State m_state = State::Prompt;
        std::int64_t m_music_sample = 0;
        std::int64_t m_sample_rate = 0;
    };
}
