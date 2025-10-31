#pragma once
#define NO_S3D_USING
#include <Siv3D.hpp>

#include "Constant.hpp"

namespace SheetsAnalyzer {
    struct HispeedData {
        s3d::int64 sample;
        double speed = 1.0;
    };

    struct ShortNoteData {
        size_t timeline_index = 0;
        s3d::int64 sample;
        s3d::uint8 start_lane = 0;
        s3d::uint8 width = 0;
    };

    enum class HoldNoteType : s3d::uint8 {
        Start,
        End,
        Mid,
        MidUnvisible,
    };

    struct HoldNoteDataInternal {
        size_t timeline_index = 0;
        s3d::int64 sample;
        s3d::uint8 start_lane = 0;
        s3d::uint8 width = 0;
        HoldNoteType type;
    };

    struct HoldNoteData {
        // judgement
        s3d::Array<HoldNoteDataInternal> judge;
        // notes array
        s3d::Array<HoldNoteDataInternal> notes;

        void clear() {
            judge.clear();
            notes.clear();
        }
    };

    struct TimelineData {
        s3d::Array<HispeedData> hispeed_data;
    };

    struct BPMData {
        s3d::int64 sample;
        double bpm = Constant::DefaultBPM;
    };

    struct SheetData {
        SheetData() : valid(false) {}

        bool valid = false;

        s3d::Array<TimelineData> timelines;
        s3d::Array<BPMData> bpm_changes;
        struct NoteData {
            s3d::Array<ShortNoteData> tap;
            s3d::Array<ShortNoteData> xtap;
            s3d::Array<ShortNoteData> flick;
            s3d::Array<HoldNoteData> hold;
        } notes;

        size_t total_combo = 0;
    };
}
