#pragma once

#include <chart_converter.h>
#include "SheetsAnalyzer.hpp"

#include <span>

namespace xlair::sheets::chart {
    class TimingMap {
    public:
        [[nodiscard]]
        static Result<TimingMap> Build(std::span<const CcTempo> tempos, const ChartLoadOptions& options);

        [[nodiscard]]
        s3d::int64 toSample(CcPosition position) const;

    private:
        struct Segment {
            long double beat = 0.0L;
            long double sample = 0.0L;
            long double samples_per_beat = 0.0L;
        };

        explicit TimingMap(s3d::Array<Segment> segments);
        s3d::Array<Segment> m_segments;
    };
}
