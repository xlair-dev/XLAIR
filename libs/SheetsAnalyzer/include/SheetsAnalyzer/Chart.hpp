#pragma once

#include <Siv3D.hpp>
#include <cstddef>

namespace xlair::sheets {
    using TimelineIndex = std::size_t;

    struct LaneSpan {
        s3d::uint8 start = 0;
        s3d::uint8 width = 1;
    };

    struct SpeedChange {
        s3d::int64 sample = 0;
        double multiplier = 1.0;
    };

    struct Timeline {
        s3d::Array<SpeedChange> speed_changes;
    };

    struct TempoChange {
        s3d::int64 sample = 0;
        double bpm = 120.0;
    };

    // Notes played on the slider.
    enum class SliderNoteKind : s3d::uint8 {
        Tap,
        XTap,
        Flick,
    };

    struct SliderNote {
        SliderNoteKind kind = SliderNoteKind::Tap;
        TimelineIndex timeline = 0;
        s3d::int64 sample = 0;
        LaneSpan lane;
    };

    enum class SliderHoldPointKind : s3d::uint8 {
        Start,
        End,
        Visible,
        Invisible,
    };

    struct SliderHoldPoint {
        SliderHoldPointKind kind = SliderHoldPointKind::Visible;
        TimelineIndex timeline = 0;
        s3d::int64 sample = 0;
        LaneSpan lane;
    };

    struct SliderHoldJudgePoint {
        s3d::int64 sample = 0;
        LaneSpan lane;
    };

    struct SliderHold {
        s3d::Array<SliderHoldPoint> points;
        s3d::Array<SliderHoldJudgePoint> judge_points;
    };

    // Physical buttons around the slider.
    enum class SideButton : s3d::uint8 {
        LeftUpper,
        RightUpper,
        LeftLower,
        RightLower,
    };

    struct SideNote {
        TimelineIndex timeline = 0;
        s3d::int64 sample = 0;
        SideButton button = SideButton::LeftUpper;
    };

    enum class SideHoldPointKind : s3d::uint8 {
        Start,
        End,
        Relay,
    };

    struct SideHoldPoint {
        SideHoldPointKind kind = SideHoldPointKind::Relay;
        TimelineIndex timeline = 0;
        s3d::int64 sample = 0;
    };

    struct SideHold {
        SideButton button = SideButton::LeftUpper;
        s3d::Array<SideHoldPoint> points;
        s3d::Array<s3d::int64> judge_samples;
    };

    struct Chart {
        s3d::int64 sample_rate = 44'100;

        s3d::Array<Timeline> timelines;
        s3d::Array<TempoChange> tempo_changes;

        s3d::Array<SliderNote> slider_notes;
        s3d::Array<SliderHold> slider_holds;

        s3d::Array<SideNote> side_notes;
        s3d::Array<SideHold> side_holds;

        std::size_t total_combo = 0;
    };
}
