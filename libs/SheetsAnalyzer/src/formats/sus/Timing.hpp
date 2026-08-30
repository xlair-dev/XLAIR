#pragma once
#define NO_S3D_USING

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
        [[nodiscard]] double bpmAt(const Position& position) const;

    private:
        struct Segment {
            Position position;
            long double beat = 0.0L;
            long double sample = 0.0L;
            double bpm = 120.0;
        };

        s3d::int64 sample_rate_ = 44'100;
        s3d::Array<Segment> segments_;
    };
}
