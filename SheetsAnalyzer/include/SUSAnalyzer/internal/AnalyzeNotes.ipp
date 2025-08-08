#pragma once
#include "SUSAnalyzer/SuSData.hpp"
#include "SUSAnalyzer/internal/Utils.ipp"

namespace SheetsAnalyzer::SUSAnalyzer::internal {
    inline void AnalyzeSpecialNotes(SUSData& data, const s3d::uint32 meas, s3d::StringView lane, s3d::StringView pattern) {
        const auto kind = lane[0];

        switch (kind) {
            case U'2':
                // Measure length
                data.beats_difinitions[meas] = s3d::ParseFloat<float>(pattern);
                break;
            case U'8': {
                // BPM
                const auto note_count = static_cast<s3d::uint32>(pattern.length()) / 2;
                const auto step = static_cast<s3d::uint32>(data.ticks_per_beat * GetBeatsAt(data, meas)) / (!note_count ? 1 : note_count);

                for (size_t i = 0; i < note_count; i++) {
                    const auto note = pattern.substr(i * 2, 2);
                    const auto ticks = static_cast<s3d::uint32>(step * i);
                    SUSRawNoteData note_data;
                    note_data.time = { meas, ticks };
                    note_data.type = NoteType::BpmChange;
                    note_data.definition_index = s3d::ParseInt<uint16>(note, 36);
                    data.raw_notes.push_back(note_data);
                }
            }   break;
            default:
                // Error: Unknown note type.
                break;
        }
    }

    inline void AnalyzeShortNotes(SUSData& data, const s3d::uint32 meas, s3d::StringView lane, s3d::StringView pattern) {
        const auto note_count = static_cast<s3d::uint32>(pattern.length()) / 2;
        const auto start_lane = s3d::ParseInt<uint8>(lane, 16);

        const auto step = static_cast<s3d::uint32>(data.ticks_per_beat * GetBeatsAt(data, meas)) / (!note_count ? 1 : note_count);

        for (size_t i = 0; i < note_count; i++) {
            const auto note = pattern.substr(i * 2, 2);
            const auto ticks = static_cast<s3d::uint32>(step * i);
            const auto kind = note[0];
            const auto width = s3d::ParseInt<uint8>(note.substr(1), 32);

            SUSRawNoteData note_data;
            note_data.time = { meas, ticks };
            note_data.NotePosition.start_lane = start_lane;
            note_data.NotePosition.width = width;
            note_data.timeline_index = data.current_timeline;

            switch (kind) {
                case U'0': // No placement
                    // Do nothing
                    break;
                case U'1': // Tap note
                    note_data.type = NoteType::TapNote;
                    break;
                case U'2': // X-Tap note
                    note_data.type = NoteType::XTapNote;
                    break;
                case U'3': // Flick note
                    note_data.type = NoteType::FlickNote;
                    break;
                case U'4': // Damage note
                    break; // Not supported
                default: // Error: Unknown note type.
                    break;
            }

            data.raw_notes.push_back(note_data);

        }
    }

    inline void AnalyzeHoldNotes(SUSData& data, const s3d::uint32 meas, s3d::StringView lane, s3d::StringView pattern) {
        const auto note_count = pattern.length() / 2;
        const auto start_lane = s3d::ParseInt<uint32>(lane, 16);

    }

}
