#include "SUSAnalyzer/internal/AnalyzeDataLine.hpp"
#include "SUSAnalyzer/internal/Utils.ipp"

namespace SheetsAnalyzer::SUSAnalyzer::internal {
    void AnalyzeDataLine(SUSData& data, const s3d::String& line) {
        const auto separator = line.indexOf(U':');
        if (separator == s3d::String::npos) {
            // Error: Invalid SUS data line.
            return;
        }

        const auto header = line.substr(1, separator - 1).trimmed().uppercased();
        const auto value = line.substr(separator + 1).filter([](const auto& c) {
            return not s3d::IsSpace(c);
        });

        const auto cmd = header.substrView(0, 3);

        if (IsNoteData(cmd)) {
            const auto note_type = header[3];
            const auto meas = s3d::ParseInt<s3d::uint32>(cmd, 10) + data.measure_base;
            const auto lane = header.substrView(4);
            switch (note_type) {
                case U'0': AnalyzeSpecialNotes(data, meas, lane, value); break;
                case U'1': AnalyzeShortNotes(data, meas, lane, value); break;
                case U'2': AnalyzeSideLongNotes(data, meas, lane, value); break;
                case U'3': AnalyzeHoldNotes(data, meas, lane, value); break;
                case U'4': 
                    // Hold2
                    break;
                case U'5': 
                    // Side Notes
                    break;
                default:
                    // Error: Unknown note type.
                    break;
            }
        } else {
            // Special data
            // TODO: Error handling
            const auto attr = header.substrView(3);
            const auto number = s3d::ParseInt<s3d::uint32>(attr, 36);
            if (cmd == U"BPM") {
                // BPM definition
                const auto bpm = s3d::ParseFloat<double>(value);
                data.bpm_difinitions[number] = bpm;
            } else if (cmd == U"TIL") {
                // Hispeed
                const auto timeline = ParseRawString(value).removed_if([](const auto& c) {
                    return s3d::IsSpace(c);
                }).split(U',');
                if (not data.hispeed_difinitions.contains(number)) {
                    data.hispeed_difinitions[number] = SUSHispeedTimeline {};
                    data.hispeed_difinitions[number].data_index = data.timelines.size() + 1;
                }
                for (const auto& arg : timeline) {
                    if (auto hispeed_data = ParseHispeedData(arg)) {
                        data.hispeed_difinitions[number].addData(*hispeed_data);
                    } else {
                        // Error: Invalid HISPEED format.
                        return;
                    }
                }
            } else if (cmd == U"ATR") {
                // note attribute
            } else {
            }
        }
    }
}
