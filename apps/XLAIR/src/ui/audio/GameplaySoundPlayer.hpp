#pragma once

#include "Common.hpp"

#include "GameplaySoundScheduler.hpp"
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
        /// Pass the uncorrected music position and each advance() result, including empty results.
        void update(
            int64 music_sample,
            const core::judgement::JudgeEngine& judge,
            std::span<const core::judgement::JudgementEvent> events,
            std::span<const std::size_t> flick_contacts = {}
        );
        /// Stops loops while allowing successful one-shot tails to finish.
        void finish();
        void stop();

    private:
        void syncLoop(GameplaySoundKind sound, bool active);

        std::array<Audio, static_cast<std::size_t>(GameplaySoundKind::Count)> m_sounds;
        GameplaySoundScheduler m_scheduler;
    };
}
