#pragma once

#include "SheetsAnalyzer.hpp"
#include <chart_converter.h>

namespace xlair::sheets::compilation {
    class TimingMap {
    public:
        [[nodiscard]]
        static Result<TimingMap> Build(const CcChartView& source, const ChartLoadOptions& options);
        [[nodiscard]]
        s3d::int64 toSample(CcPosition position) const;
        [[nodiscard]]
        s3d::Array<TempoChange> tempoChanges() const;

    private:
        struct Segment {
            long double beat = 0.0L;
            long double sample = 0.0L;
            double bpm = 120.0;
        };
        s3d::int64 m_sample_rate = 44'100;
        s3d::Array<Segment> m_segments;
    };
}
