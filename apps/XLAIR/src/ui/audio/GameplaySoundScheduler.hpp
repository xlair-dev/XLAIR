#pragma once

#include "core/judgement/JudgeEngine.hpp"

#include <queue>
#include <vector>

namespace xlair::ui::audio {
    enum class GameplaySoundKind : std::size_t {
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

    // Plans playback without touching audio devices or changing judgement timing.
    class GameplaySoundScheduler {
    public:
        struct Frame {
            Array<GameplaySoundKind> one_shots;
            bool slider_hold_active = false;
            bool side_hold_active = false;
        };

        void start(const sheets::Chart& chart);

        // music_sample is the uncorrected playback position, not the judgement clock.
        // Consume each advance() result once; empty results also release queued sounds.
        [[nodiscard]]
        Frame update(
            int64 music_sample,
            const core::judgement::JudgeEngine& judge,
            std::span<const core::judgement::JudgementEvent> events
        );

        void clear();

    private:
        struct Cue {
            GameplaySoundKind sound;
            int64 sample = 0;
        };

        struct HoldSounds {
            int64 start_sample = 0;
            Array<Optional<Cue>> points;
        };

        struct LaterCue {
            bool operator()(const Cue& left, const Cue& right) const {
                return left.sample > right.sample;
            }
        };

        Array<Cue> m_slider_notes;
        Array<Cue> m_side_notes;
        Array<HoldSounds> m_slider_holds;
        Array<HoldSounds> m_side_holds;
        std::priority_queue<Cue, std::vector<Cue>, LaterCue> m_pending;
    };
}
