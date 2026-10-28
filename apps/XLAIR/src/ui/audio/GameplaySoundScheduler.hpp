#pragma once

#include "core/judgement/JudgeEngine.hpp"

#include <array>
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
            std::span<const core::judgement::JudgementEvent> events,
            std::span<const std::size_t> flick_contacts = {}
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
        Array<bool> m_slider_note_scheduled;
        Array<Cue> m_side_notes;
        Array<HoldSounds> m_slider_holds;
        Array<HoldSounds> m_side_holds;
        std::array<HashSet<int64>, static_cast<std::size_t>(GameplaySoundKind::Count)> m_played_samples;
        std::priority_queue<Cue, std::vector<Cue>, LaterCue> m_pending;
    };
}
