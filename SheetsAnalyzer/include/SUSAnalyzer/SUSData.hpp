#pragma once
#define NO_S3D_USING
#include <Siv3D.hpp>

#include "SheetData.hpp"
#include "Constant.hpp"

namespace SheetsAnalyzer::SUSAnalyzer {
    namespace Constant {
        constexpr inline float DefaultBeatLength = 4.0f;
        constexpr inline s3d::uint32 DefaultHispeedNumber = std::numeric_limits<s3d::uint32>::max();
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
        s3d::uint32 timeline_index = Constant::DefaultHispeedNumber;
        union {
            s3d::uint16 definition_index = 0;
            struct {
                s3d::uint8 start_lane;
                s3d::uint8 width;
            } NotePosition;
        };
    };

    struct SUSHispeedData {
        SUSRelativeNoteTime time;
        double speed = 1.0f; // Default speed is 1.0x
    };

    struct SUSHispeedTimeline {
        s3d::Array<SUSHispeedData> hispeed_data;
        inline void reset() {
            hispeed_data.clear();
        }
        inline void addData(const SUSHispeedData& data) {
            hispeed_data.push_back(data);
        }
    };;

    struct SUSData : SheetData {

        SUSData() {
            reset();
        }

        inline void reset() {
            valid = false;
            ticks_per_beat = 480;
            bpm_difinitions.clear();
            beats_difinitions.clear();
            hispeed_difinitions.clear();
            measure_base = 0;

            bpm_difinitions[0] = 120.0; // Default BPM
            beats_difinitions[0] = Constant::DefaultBeatLength;
            hispeed_difinitions[Constant::DefaultHispeedNumber].addData({
                .time = { 0, 0 },
                .speed = 1.0f // Default speed is 1.0x
            });
            current_timeline = Constant::DefaultHispeedNumber;
            measure_timeline = Constant::DefaultHispeedNumber;
        }

        s3d::uint32 current_timeline = Constant::DefaultHispeedNumber;
        s3d::uint32 measure_base = 0;
        s3d::uint32 measure_timeline = Constant::DefaultHispeedNumber;
        s3d::uint32 ticks_per_beat = 480;
        s3d::Array<SUSRawNoteData> raw_notes;
        s3d::HashTable<s3d::uint32, double> bpm_difinitions;
        s3d::HashTable<s3d::uint32, SUSHispeedTimeline> hispeed_difinitions;
        std::map<s3d::uint32, float> beats_difinitions;
    };
}
