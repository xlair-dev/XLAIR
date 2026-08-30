#pragma once

#include <Siv3D.hpp>

#include "SheetsAnalyzer/Result.hpp"
#include "Document.hpp"

namespace xlair::sheets::formats::sus {
    struct TimingOptions {
        s3d::int64 sample_rate = 44'100;
        double offset_seconds = 0.0;
    };

    class TimingMap {
    public:
        [[nodiscard]] static Result<TimingMap> Build(const Document& document, const TimingOptions& options);

        [[nodiscard]] s3d::int64 toSample(const Position& position) const;
        [[nodiscard]] s3d::int64 toSample(const TickPosition& position) const;
        [[nodiscard]] double bpmAt(const Position& position) const;

    private:
        struct MeasureSegment {
            s3d::uint32 measure = 0;
            long double beat = 0.0L;
            long double beats_per_measure = 4.0L;
        };

        struct Segment {
            long double beat = 0.0L;
            long double sample = 0.0L;
            double bpm = 120.0;
        };

        TimingMap(s3d::int64 sample_rate, s3d::uint32 ticks_per_beat, long double offset_samples);

        [[nodiscard]] const MeasureSegment& measureSegmentAt(s3d::uint32 measure) const;
        [[nodiscard]] const Segment& segmentAt(long double beat) const;
        [[nodiscard]] s3d::int64 sampleAt(long double beat) const;
        [[nodiscard]] long double absoluteBeat(const Position& position) const;
        [[nodiscard]] long double absoluteBeat(const TickPosition& position) const;

        s3d::int64 m_sample_rate = 44'100;
        s3d::uint32 m_ticks_per_beat = 480;
        s3d::Array<MeasureSegment> m_measure_segments;
        s3d::Array<Segment> m_segments;
    };
}
