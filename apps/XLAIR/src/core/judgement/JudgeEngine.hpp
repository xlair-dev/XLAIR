#pragma once

#include "Common.hpp"

#include <SheetsAnalyzer/Chart.hpp>

#include <span>

namespace xlair::core::judgement {
    enum class Grade : uint8 {
        Perfect,
        Great,
        Good,
        Miss,
    };

    enum class NoteState : uint8 {
        Pending,
        Hit,
        Missed,
    };

    struct TouchEvent {
        enum class Phase : uint8 {
            Down,
            Up,
        };

        uint8 zone = 0;
        int64 sample = 0;
        Phase phase = Phase::Down;
    };

    struct SideEvent {
        sheets::SideButton button = sheets::SideButton::LeftUpper;
        int64 sample = 0;
    };

    enum class NoteKind : uint8 {
        Slider,
        Side,
        SliderHold,
        SideHold,
    };

    struct JudgementEvent {
        NoteKind note_kind = NoteKind::Slider;
        std::size_t note_index = 0;
        std::size_t point_index = 0;
        Grade grade = Grade::Miss;
        int64 offset_samples = 0;
        /// A hold acquired through its co-located XTap, rather than an independent input judgement.
        bool linked_start = false;
    };

    struct ScoreState {
        uint32 perfect_count = 0;
        uint32 great_count = 0;
        uint32 good_count = 0;
        uint32 miss_count = 0;
        uint32 combo = 0;
        uint32 max_combo = 0;
        uint32 score = 0;
        double clear_gauge = 0.0;
    };

    class JudgeEngine {
    public:
        /// The compiled chart, including hold paths, must remain unchanged and outlive this engine.
        explicit JudgeEngine(const sheets::Chart& chart);

        [[nodiscard]]
        Array<JudgementEvent> advance(
            int64 current_sample,
            std::span<const TouchEvent> touches = {},
            std::span<const SideEvent> side_down = {},
            uint32 slider_pressed = 0,
            uint8 side_pressed = 0
        );

        [[nodiscard]]
        NoteState noteState(std::size_t slider_note_index) const;

        [[nodiscard]]
        NoteState sideNoteState(std::size_t side_note_index) const;

        [[nodiscard]]
        NoteState sliderHoldPointState(std::size_t hold_index, std::size_t point_index) const;

        [[nodiscard]]
        NoteState sideHoldPointState(std::size_t hold_index, std::size_t point_index) const;

        /// Activity reflects the last advance: an acquired hold, current contact, and an unjudged end.
        [[nodiscard]]
        bool sliderHoldActive(std::size_t hold_index) const;

        [[nodiscard]]
        bool sideHoldActive(std::size_t hold_index) const;

        [[nodiscard]]
        const ScoreState& score() const noexcept;

    private:
        struct FlickGesture {
            uint8 contact_zone = 0;
            int64 contact_sample = 0;
            Optional<uint8> motion_zone;
            int64 motion_sample = 0;
            bool preheld_motion = false;
        };

        struct Note {
            NoteKind note_kind = NoteKind::Slider;
            std::size_t chart_index = 0;
            std::size_t point_index = 0;
            int64 sample = 0;
            sheets::LaneSpan lane;
            sheets::SliderNoteKind slider_kind = sheets::SliderNoteKind::Tap;
            sheets::SideButton side_button = sheets::SideButton::LeftUpper;
            sheets::SideNoteKind side_kind = sheets::SideNoteKind::Tap;
            NoteState state = NoteState::Pending;
            Optional<FlickGesture> flick_gesture;

            [[nodiscard]]
            bool isXTap() const noexcept;
        };

        struct HoldPoint {
            NoteKind note_kind = NoteKind::SliderHold;
            std::size_t hold_index = 0;
            std::size_t point_index = 0;
            int64 sample = 0;
            sheets::LaneSpan lane;
            sheets::SideButton side_button = sheets::SideButton::LeftUpper;
            NoteState state = NoteState::Pending;
        };

        struct HoldPointIndex {
            NoteKind note_kind = NoteKind::SliderHold;
            std::size_t hold = 0;
            std::size_t point = 0;
        };

        struct Hold {
            Array<HoldPoint> points;
            bool acquired = false;
            bool active = false;
        };

        [[nodiscard]]
        bool acceptsBoundary(const Note& candidate, uint8 lane, int64 sample, Grade grade) const;
        [[nodiscard]]
        bool acceptsSideBoundary(const Note& candidate, int64 sample) const;
        [[nodiscard]]
        Grade gradeFor(const Note& note, int64 sample) const;
        [[nodiscard]]
        HoldPoint& holdPoint(HoldPointIndex index);
        void processFlickTouch(const TouchEvent& touch, uint32 slider_pressed, Array<JudgementEvent>& events);
        [[nodiscard]]
        Grade flickGrade(const Note& note) const;
        void applyGrade(Grade grade);
        void record(Note& note, Grade grade, int64 sample, Array<JudgementEvent>& events, bool linked_start = false);
        void record(HoldPoint& point, Grade grade, int64 sample, Array<JudgementEvent>& events);
        void updateHoldActivity(int64 sample, uint32 slider_pressed, uint8 side_pressed);

        const sheets::Chart& m_chart;
        Array<Note> m_notes;
        Array<std::size_t> m_order;
        Array<std::size_t> m_flick_order;
        Array<Hold> m_slider_holds;
        Array<Hold> m_side_holds;
        Array<HoldPointIndex> m_hold_order;
        std::size_t m_slider_note_count = 0;
        std::size_t m_side_note_count = 0;
        std::size_t m_miss_cursor = 0;
        std::size_t m_flick_miss_cursor = 0;
        std::size_t m_hold_cursor = 0;
        ScoreState m_score;
        std::size_t m_total_combo = 0;
        int64 m_perfect_window = 0;
        int64 m_great_window = 0;
        int64 m_good_window = 0;
        int64 m_flick_tap_window = 0;
        int64 m_flick_slide_window = 0;
        uint32 m_previous_slider_pressed = 0;
    };
}
