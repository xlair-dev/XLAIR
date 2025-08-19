#pragma once
#define NO_S3D_USING
#include <Siv3D.hpp>

#include "Constant.hpp"

namespace SheetsAnalyzer {
    struct HispeedData {
        s3d::uint64 sample;
        double speed = 1.0;
    };

    struct ShortNoteData {
        size_t timeline_index = 0;
        s3d::uint64 sample;
    };

    struct TimelineData {
        s3d::Array<HispeedData> hispeed_data;
    };

    struct SheetData {
        bool valid = false;

        s3d::Array<TimelineData> timelines;
        struct NoteData {
            s3d::Array<ShortNoteData> tap;
            s3d::Array<ShortNoteData> xtap;
            s3d::Array<ShortNoteData> flick;
        } notes;
    };
}
