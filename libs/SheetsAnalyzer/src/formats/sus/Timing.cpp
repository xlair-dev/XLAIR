#include "Timing.hpp"

namespace xlair::sheets::formats::sus {
    Result<TimingMap> TimingMap::Build(const Document& document, const TimingOptions& options) {
        (void)document;
        (void)options;

        Result<TimingMap> result;
        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::Error,
            .message = U"TimingMap has not been migrated yet.",
            .path = U"",
            .line = s3d::none,
            .column = s3d::none,
        });
        return result;
    }

    s3d::int64 TimingMap::toSample(const Position& position) const {
        (void)position;
        return 0;
    }

    double TimingMap::bpmAt(const Position& position) const {
        (void)position;
        return 120.0;
    }
}
