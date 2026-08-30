#pragma once

#include <SheetsAnalyzer/Chart.hpp>

namespace xlair::playfield {
    class ScrollTimeline {
    public:
        explicit ScrollTimeline(const sheets::Timeline& timeline, s3d::int64 sample_rate);

        [[nodiscard]]
        long double positionAt(s3d::int64 sample) const;

        [[nodiscard]]
        long double distanceBetween(s3d::int64 from_sample, s3d::int64 to_sample) const;

    private:
        struct Segment {
            s3d::int64 sample = 0;
            double multiplier = 1.0;
            long double position = 0.0L;
        };

        s3d::int64 m_sample_rate = 44'100;
        s3d::Array<Segment> m_segments;
    };
}
