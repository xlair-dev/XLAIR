#pragma once

#include <Siv3D.hpp>

namespace xlair::sheets::formats::sus {
    using DefinitionId = s3d::uint32;
    using ChannelId = s3d::uint32;
    using TimelineId = s3d::uint32;

    // Exact fractional position inside a measure.
    // Keeping the fraction here prevents the parser from rounding subdivisions to integer ticks.
    struct Position {
        s3d::uint32 measure = 0;
        s3d::uint32 numerator = 0;
        s3d::uint32 denominator = 1;
    };

    struct LaneSpan {
        s3d::uint8 start = 0;
        s3d::uint8 width = 1;
    };

    enum class SliderNoteKind : s3d::uint8 {
        Tap,
        XTap,
        Flick,
    };

    struct SliderNote {
        SliderNoteKind kind = SliderNoteKind::Tap;
        Position position;
        LaneSpan lane;
        s3d::Optional<TimelineId> timeline;
    };

    enum class SliderHoldPointKind : s3d::uint8 {
        Start,
        End,
        Visible,
        Invisible,
        Control,
    };

    struct SliderHoldPoint {
        SliderHoldPointKind kind = SliderHoldPointKind::Visible;
        Position position;
        LaneSpan lane;
        ChannelId channel = 0;
        s3d::Optional<TimelineId> timeline;
    };

    // SUS #mmm2xy.
    // XLAIR uses this channel for SideLong notes.
    //
    // x is the lane and y identifies the long-note channel.
    // A Start point (1?) selects one of the four physical side buttons from its lane.
    // Relay/End points belong to that same button; their lanes do not select the button again.
    // The parser keeps this source representation intact and the compiler performs the physical-button mapping.
    enum class SideLongPointKind : s3d::uint8 {
        Start,
        End,
        Relay,
    };

    struct SideLongPoint {
        SideLongPointKind kind = SideLongPointKind::Relay;
        Position position;
        LaneSpan lane;
        ChannelId channel = 0;
        s3d::Optional<TimelineId> timeline;
    };

    // SUS #mmm5x directional notes.
    // These remain SUS concepts until compile.
    enum class DirectionalKind : s3d::uint8 {
        Up,
        Down,
        LeftUp,
        RightUp,
        LeftDown,
        RightDown,
    };

    struct DirectionalNote {
        DirectionalKind kind = DirectionalKind::Up;
        Position position;
        LaneSpan lane;
        s3d::Optional<TimelineId> timeline;
    };

    struct BPMReference {
        Position position;
        DefinitionId definition = 0;
    };

    struct HispeedChange {
        Position position;
        double multiplier = 1.0;
    };

    struct HispeedDefinition {
        s3d::Array<HispeedChange> changes;
    };

    struct Document {
        s3d::uint32 ticks_per_beat = 480;

        s3d::HashTable<DefinitionId, double> bpm_definitions;
        s3d::Array<BPMReference> bpm_changes;
        s3d::HashTable<s3d::uint32, double> beats_per_measure;

        s3d::HashTable<TimelineId, HispeedDefinition> hispeed_definitions;

        s3d::Array<SliderNote> slider_notes;
        s3d::Array<SliderHoldPoint> slider_hold_points;
        s3d::Array<SideLongPoint> side_long_points;
        s3d::Array<DirectionalNote> directional_notes;
    };
}
