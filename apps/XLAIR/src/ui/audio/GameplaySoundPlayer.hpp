#pragma once

#include "Common.hpp"

#include "core/judgement/JudgeEngine.hpp"
#include <array>

namespace xlair::ui::audio {
    /// Plays successful judgement feedback and the judge engine's active hold loops.
    class GameplaySoundPlayer {
    public:
        GameplaySoundPlayer();
        ~GameplaySoundPlayer();

        GameplaySoundPlayer(const GameplaySoundPlayer&) = delete;
        GameplaySoundPlayer& operator=(const GameplaySoundPlayer&) = delete;

        /// Use the engine's compiled chart, whose judgement points are sorted by sample.
        void start(const sheets::Chart& chart);
        /// Consume each advance() result once, including empty results, to synchronize released holds.
        void update(const core::judgement::JudgeEngine& judge, std::span<const core::judgement::JudgementEvent> events);
        /// Stops loops while allowing successful one-shot tails to finish.
        void finish();
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

        void syncLoop(Sound sound, bool active);

        std::array<Audio, static_cast<std::size_t>(Sound::Count)> m_sounds;
        Array<Sound> m_slider_notes;
        Array<Sound> m_side_notes;
        Array<Array<Optional<Sound>>> m_slider_holds;
        Array<Array<Optional<Sound>>> m_side_holds;
    };
}
