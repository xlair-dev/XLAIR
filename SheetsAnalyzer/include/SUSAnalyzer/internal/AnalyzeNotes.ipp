#pragma once
#include "SUSAnalyzer/SUSData.hpp"
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
                const auto step = static_cast<s3d::uint32>(data.ticks_per_beat * data.getBeatsAt(meas)) / (!note_count ? 1 : note_count);

                for (size_t i = 0; i < note_count; i++) {
                    const auto note = pattern.substr(i * 2, 2);
                    const auto ticks = static_cast<s3d::uint32>(step * i);
                    SUSRawNoteData note_data;
                    note_data.time = { meas, ticks };
                    note_data.type = NoteType::BpmChange;
                    note_data.definition_index = s3d::ParseInt<s3d::uint16>(note, 36);
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
        const auto start_lane = s3d::ParseInt<s3d::uint8>(lane, 16);

        const auto step = static_cast<s3d::uint32>(data.ticks_per_beat * data.getBeatsAt(meas)) / (!note_count ? 1 : note_count);

        for (size_t i = 0; i < note_count; i++) {
            const auto note = pattern.substr(i * 2, 2);
            const auto ticks = static_cast<s3d::uint32>(step * i);
            const auto kind = note[0];
            const auto width = s3d::ParseInt<s3d::uint8>(note.substr(1), 32);

            SUSRawNoteData note_data;
            note_data.time = { meas, ticks };
            note_data.note_position.start_lane = start_lane;
            note_data.note_position.width = width;
            note_data.timeline_index = data.current_timeline;

            switch (kind) {
                case U'0': // No placement
                    continue; // Do nothing and skip
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

    inline void AnalyzeSideLongNotes(SUSData& data, const s3d::uint32 meas, s3d::StringView lane, s3d::StringView pattern) {
        if (lane.size() != 2) {
            // Error: Invalid lane for hold note.
            return;
        }

        const auto note_count = pattern.length() / 2;
        const auto start_lane = s3d::ParseInt<s3d::uint32>(lane.substr(0, 1), 16);
        const auto note_channel = s3d::ParseInt<s3d::uint32>(lane.substr(1), 16);

        // TODO: Implement side long notes
    }

    inline void AnalyzeHoldNotes(SUSData& data, const s3d::uint32 meas, s3d::StringView lane, s3d::StringView pattern) {
        if (lane.size() != 2) {
            // Error: Invalid lane for hold note.
            return;
        }
        const auto note_count = pattern.length() / 2;
        const auto start_lane = s3d::ParseInt<s3d::uint32>(lane.substr(0, 1), 16);
        const auto note_channel = s3d::ParseInt<s3d::uint32>(lane.substr(1), 16);

        const auto step = static_cast<s3d::uint32>(data.ticks_per_beat * data.getBeatsAt(meas)) / (!note_count ? 1 : note_count);

        for (size_t i = 0; i < note_count; i++) {
            const auto note = pattern.substr(i * 2, 2);
            const auto ticks = static_cast<s3d::uint32>(step * i);
            const auto kind = note[0];
            const auto width = s3d::ParseInt<s3d::uint8>(note.substr(1), 32);

            SUSRawNoteData note_data;
            note_data.time = { meas, ticks };
            note_data.note_position.start_lane = start_lane;
            note_data.note_position.width = width;
            note_data.timeline_index = data.current_timeline;
            note_data.extra = note_channel;

            switch (kind) {
                case U'0': // No placement
                    continue; // Do nothing and skip
                    break;
                case U'1': // Hold start
                    note_data.type = NoteType::HoldStartNote;
                    break;
                case U'2': // Hold end
                    note_data.type = NoteType::HoldEndNote;
                    break;
                case U'3': // Hold mid
                    note_data.type = NoteType::HoldMidNote;
                    break;
                case U'4': // Hold control point
                    note_data.type = NoteType::HoldControlPoint;
                    break;
                case U'5': // Hold mid invisible
                    note_data.type = NoteType::HoldMidUnvisibleNote;
                    break;
                default: // Error: Unknown note type.
                    break;
            }

            data.raw_notes.push_back(note_data);
        }
    }

}
