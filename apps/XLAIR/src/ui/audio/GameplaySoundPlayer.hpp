#pragma once

#include "Common.hpp"

#include <SheetsAnalyzer/Chart.hpp>
#include <array>

namespace xlair::ui::audio {
    /// Plays chart-timed sounds independently of input judgement.
    /// Samples use the compiled chart's music offset and audio sample rate, not scroll timelines.
    class GameplaySoundPlayer {
    public:
        GameplaySoundPlayer();
        ~GameplaySoundPlayer();

        GameplaySoundPlayer(const GameplaySoundPlayer&) = delete;
        GameplaySoundPlayer& operator=(const GameplaySoundPlayer&) = delete;

        /// Uses explicit hold points for sounds, independently of generated judgement ticks.
        /// The chart must be compiled at the sample rate of the playing music.
        void start(const sheets::Chart& chart);
        /// Advances with the music's monotonically increasing sample position, starting at zero.
        void update(int64 sample);
        /// Processes the music endpoint and stops loops, allowing one-shot tails to finish.
        void finish(int64 sample);
        void stop();

    private:
        enum class Sound : std::size_t {
            Tap,
            SlideTap,
            XTap,
            Flick,
            SideTap,
            Checkpoint,
            SideHold,
            SlideHold,
            Count,
        };

        struct Event {
            int64 sample;
            Sound sound;
            int32 change = 0;
        };

        std::array<Audio, static_cast<std::size_t>(Sound::Count)> m_sounds;
        std::array<int32, static_cast<std::size_t>(Sound::Count)> m_active_holds{};
        Array<Event> m_events;
        std::size_t m_cursor = 0;
    };
}
