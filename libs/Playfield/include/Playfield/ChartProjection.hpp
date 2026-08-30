#pragma once

#include "ScrollTimeline.hpp"

namespace xlair::playfield {
    class ChartProjection {
    public:
        explicit ChartProjection(const sheets::Chart& chart);

        [[nodiscard]]
        long double positionAt(sheets::TimelineIndex timeline, s3d::int64 sample) const;

        [[nodiscard]]
        long double distanceBetween(sheets::TimelineIndex timeline, s3d::int64 from_sample, s3d::int64 to_sample) const;

        [[nodiscard]]
        std::size_t timelineCount() const noexcept;

    private:
        s3d::Array<ScrollTimeline> m_timelines;
    };
}
