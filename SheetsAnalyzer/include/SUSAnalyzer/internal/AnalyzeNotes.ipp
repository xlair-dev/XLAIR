#pragma once
#include "SheetData.hpp"

namespace SheetsAnalyzer::SUSAnalyzer::internal {
    inline void AnalyzeSpecialNotes(SheetData& data, const s3d::uint32 meas, s3d::StringView lane, s3d::StringView pattern) {
        const auto kind = lane[0];

        switch (kind) {
            case U'2':
                // Measure length
                s3d::Print << U"Measure Length: " << meas << U" " << pattern;
                break;
            case U'8':
                // BPM
                s3d::Print << U"BPM: " << meas << U" " << pattern;
                break;
            default:
                // Error: Unknown note type.
                break;
        }
    }

    inline void AnalyzeShortNotes(SheetData& data, const s3d::uint32 meas, s3d::StringView lane, s3d::StringView pattern) {
        const auto note_count = pattern.length() / 2;
        const auto start_lane = s3d::ParseInt<uint32>(lane, 16);

        for (size_t i = 0; i < note_count; i++) {
            const auto note = pattern.substr(i * 2, 2);
            const auto kind = note[0];
            const auto width = s3d::ParseInt<uint32>(note.substr(1), 32);

            switch (kind) {
                case U'0': // No placement
                    // Do nothing
                    break;
                case U'1': // Tap note
                    s3d::Print << U"Tap: " << meas << U" " << start_lane << U" " << width;
                    break;
                case U'2': // X-Tap note
                    s3d::Print << U"ExTap: " << meas << U" " << start_lane << U" " << width;
                    break;
                case U'3': // Flick note
                    s3d::Print << U"Flick: " << meas << U" " << start_lane << U" " << width;
                    break;
                case U'4': // Damage note
                    break; // Not supported
                default: // Error: Unknown note type.
                    break;
            }

        }
    }

    inline void AnalyzeHoldNotes(SheetData& data, const s3d::uint32 meas, s3d::StringView lane, s3d::StringView pattern) {
        const auto note_count = pattern.length() / 2;
        const auto start_lane = s3d::ParseInt<uint32>(lane, 16);

    }

}
