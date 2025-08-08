#pragma once
#define NO_S3D_USING
#include <Siv3D.hpp>

#include "SheetData.hpp"
#include "Constant.hpp"

namespace SheetsAnalyzer::SUSAnalyzer {
    namespace Constant {
        constexpr inline float DefaultBeatLength = 4.0f;
    }

    struct SUSRelativeNoteTime {
        s3d::uint32 measure;
        s3d::uint32 ticks;

        auto operator<=>(const SUSRelativeNoteTime& other) const = default;
    };

    enum class NoteType {
        Undefined = 0,
        BpmChange,

        // Short notes
        TapNote,
        XTapNote,
        FlickNote,
    };

    struct SUSRawNoteData {
        NoteType type = NoteType::Undefined;
        SUSRelativeNoteTime time;
        union {
            s3d::uint16 definition_index = 0;
            struct {
                s3d::uint8 start_lane;
                s3d::uint8 width;
            } NotePosition;
        };
    };

    struct SUSData : SheetData {

        SUSData() {
            reset();
        }

        inline void reset() {
            valid = false;
            ticks_per_beat = 480;
            bpm_difinitions.clear();
            beats_difinitions.clear();

            bpm_difinitions[0] = 120.0; // Default BPM
            beats_difinitions[0] = Constant::DefaultBeatLength;
        }

        s3d::uint32 ticks_per_beat = 480;
        s3d::Array<SUSRawNoteData> raw_notes;
        s3d::HashTable<s3d::uint32, double> bpm_difinitions;
        std::map<s3d::uint32, float> beats_difinitions;
    };
}
