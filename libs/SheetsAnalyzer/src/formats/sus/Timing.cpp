#include "Timing.hpp"

namespace xlair::sheets::formats::sus {
    Result<TimingMap> TimingMap::Build(const Document& document, const TimingOptions& options) {
        (void)document;
        (void)options;

        return Result<TimingMap>::makeError(U"TimingMap has not been migrated yet.");
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
