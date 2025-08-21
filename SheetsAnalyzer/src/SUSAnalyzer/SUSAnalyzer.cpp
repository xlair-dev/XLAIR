#include "SUSAnalyzer/SUSAnalyzer.hpp"
#include "SUSAnalyzer/internal/AnalyzeCommandLine.hpp"
#include "SUSAnalyzer/internal/AnalyzeDataLine.hpp"
#include "SUSAnalyzer/internal/Utils.ipp"
#include "SUSAnalyzer/SUSData.hpp"

namespace SheetsAnalyzer::SUSAnalyzer {
    s3d::Optional<SheetData> Analyze(const s3d::FilePath& path, const s3d::int64 sample_rate, const double offset_sec) {
        using namespace internal;

        s3d::TextReader reader(path);
        if (not reader) {
            return s3d::none; // Failed to open file
        }

        const auto content = reader.readLines();
        reader.close();

        const auto sample_offset = static_cast<s3d::int64>(offset_sec * sample_rate);
        SUSData data { sample_rate, sample_offset };
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
        data.raw_notes.sort_by([](const SUSRawNoteData& a, const SUSRawNoteData& b) {
            return std::tie(a.time, a.type, a.timeline_index) < std::tie(b.time, b.type, b.timeline_index);
        });

        data.bpm_notes.append(data.raw_notes.filter([](const auto t) {
            return t.type == NoteType::BpmChange;
        }));

        // Convert SUSData to SheetData
        data.convertToSheetData();
        return static_cast<SheetData>(data);
    }
}
