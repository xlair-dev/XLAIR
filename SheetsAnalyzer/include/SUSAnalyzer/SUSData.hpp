#pragma once
#define NO_S3D_USING
#include <Siv3D.hpp>

#include "SheetData.hpp"
#include "Constant.hpp"

namespace SheetsAnalyzer::SUSAnalyzer {
    namespace Constant {
        constexpr inline float DefaultBeatLength = 4.0f;
        constexpr inline s3d::uint32 DefaultHispeedNumber = std::numeric_limits<s3d::uint32>::max();
        constexpr inline double DefaultBPM = 120.0;
        constexpr inline s3d::uint32 DefaultTicksPerBeat = 480;
        constexpr inline s3d::uint64 DefaultSampleRate = 44100;
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
            } note_position;
        };
    };

    struct SUSHispeedData {
        SUSRelativeNoteTime time;
        double speed = 1.0f; // Default speed is 1.0x
    };

    struct SUSHispeedTimeline {
        size_t data_index = 0; // Use for mapping to SheetData's timelines
        s3d::Array<SUSHispeedData> hispeed_data;

        SUSHispeedTimeline() {
            reset();
        }

        inline void reset() {
            hispeed_data.clear();
            addData({
                .time = { 0, 0 },
                .speed = 1.0f // Default speed is 1.0x
            });
        }
        inline void addData(const SUSHispeedData& data) {
            hispeed_data.push_back(data);
        }
    };;

    struct SUSData : SheetData {

        SUSData();
        SUSData(const s3d::uint64 sample_rate, const s3d::uint64 sample_offset);

        void convertToSheetData();
        float getBeatsAt(const s3d::uint32 meas) const;
        s3d::uint64 getSampleAt(const SUSRelativeNoteTime& time);

        s3d::uint32 current_timeline = Constant::DefaultHispeedNumber;
        s3d::uint32 measure_base = 0;
        s3d::uint32 measure_timeline = Constant::DefaultHispeedNumber;
        s3d::uint32 ticks_per_beat = Constant::DefaultTicksPerBeat;
        s3d::Array<SUSRawNoteData> raw_notes;
        s3d::Array<SUSRawNoteData> bpm_notes;
        s3d::HashTable<s3d::uint32, double> bpm_difinitions;
        s3d::HashTable<s3d::uint32, SUSHispeedTimeline> hispeed_difinitions;
        std::map<s3d::uint32, float> beats_difinitions;

        s3d::uint64 sample_rate = Constant::DefaultSampleRate;
        s3d::uint64 sample_offset = 0;
    };
}
