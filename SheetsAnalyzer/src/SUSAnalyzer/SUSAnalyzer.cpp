#include "SUSAnalyzer/SUSAnalyzer.hpp"
#include "SUSAnalyzer/internal/AnalyzeCommandLine.hpp"
#include "SUSAnalyzer/internal/AnalyzeDataLine.hpp"
#include "SUSAnalyzer/internal/Utils.ipp"
#include "SUSAnalyzer/SUSData.hpp"

namespace SheetsAnalyzer::SUSAnalyzer {
    s3d::Optional<SheetData> Analyze(const s3d::FilePath& path) {
        using namespace internal;

        s3d::TextReader reader(path);
        if (not reader) {
            return s3d::none; // Failed to open file
        }

        const auto content = reader.readLines();
        reader.close();

        SUSData data;
        for (const auto& line : content) {
            if (line.empty()) {
                continue; // Skip empty lines
            }
            if (not line.starts_with(U"#")) {
                continue; // Skip comments
            }
            if (IsCommandLine(line)) {
                AnalyzeCommandLine(data, line);
                continue;
            }
            if (IsDataLine(line)) {
                AnalyzeDataLine(data, line);
                continue;
            }
            // Error: Invalid SUS line.
        }

        // sort

        for (const auto& note : data.raw_notes) {
            if (note.type == NoteType::TapNote) {
                s3d::Print << U"Tap Note: " << note.time.measure << U":" << note.time.ticks
                           << U" Lane: " << note.NotePosition.start_lane << U" Width: " << note.NotePosition.width
                    << U" Timeline: " << note.timeline_index;
            }
        }

        data.valid = true;
        return static_cast<SheetData>(data);
    }
}
