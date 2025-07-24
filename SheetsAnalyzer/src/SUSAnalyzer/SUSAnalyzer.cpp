#include "SUSAnalyzer/SUSAnalyzer.hpp"
#include "SUSAnalyzer/internal/AnalyzeCommandLine.hpp"
#include "SUSAnalyzer/internal/AnalyzeDataLine.hpp"
#include "SUSAnalyzer/internal/Utils.ipp"

namespace SheetsAnalyzer::SUSAnalyzer {
    s3d::Optional<SUSData> Analyze(const s3d::FilePath& path) {
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

        return data;
    }
}
