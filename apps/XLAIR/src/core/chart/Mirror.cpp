#include "Mirror.hpp"

namespace xlair::core::chart {
    namespace {
        void MirrorLane(sheets::LaneSpan& lane) {
            constexpr s3d::uint8 LaneCount = 16;
            lane.start = static_cast<s3d::uint8>(LaneCount - lane.start - lane.width);
        }

        [[nodiscard]]
        sheets::SideButton MirrorButton(const sheets::SideButton button) {
            switch (button) {
                case sheets::SideButton::LeftUpper:
                    return sheets::SideButton::RightUpper;
                case sheets::SideButton::RightUpper:
                    return sheets::SideButton::LeftUpper;
                case sheets::SideButton::LeftLower:
                    return sheets::SideButton::RightLower;
                case sheets::SideButton::RightLower:
                    return sheets::SideButton::LeftLower;
            }
            return button;
        }
    }

    void ApplyMirror(sheets::Chart& chart) {
        for (auto& note : chart.slider_notes) {
            MirrorLane(note.lane);
        }
        for (auto& hold : chart.slider_holds) {
            for (auto& point : hold.points) {
                MirrorLane(point.lane);
            }
            for (auto& point : hold.judge_points) {
                MirrorLane(point.lane);
            }
        }
        for (auto& note : chart.side_notes) {
            note.button = MirrorButton(note.button);
        }
        for (auto& hold : chart.side_holds) {
            hold.button = MirrorButton(hold.button);
        }
    }
}
