#include "SUSAnalyzer/internal/AnalyzeCommandLine.hpp"
#include "SUSAnalyzer/internal/Utils.ipp"

namespace SheetsAnalyzer::SUSAnalyzer::internal {
    void AnalyzeCommandLine(SUSData& data, const s3d::String& line) {
        const auto separator = line.indexOf(U' ');
        if (separator == s3d::String::npos) {
            // Error: Invalid SUS command line.
            return;
        }
        const auto key = line.substr(1, separator - 1).trimmed().uppercase();
        const auto value = line.substr(separator + 1).trimmed();

        // These keys are handled in metadata analyzer or ignored:
        // - TITLE
        // - SUBTITLE
        // - ARTIST
        // - GENRE
        // - DESIGNER
        // - DIFFICULTY
        // - PLAYLEVEL
        // - SONGIG
        // - WAVE
        // - WAVEOFFSET
        // - JACKET
        // - BACKGROUND
        // - MOVIE
        // - MOVIEOFFSET
        // - BASEBPM
        // so, we ignore them here.
        if (
            key == U"TITLE" or
            key == U"SUBTITLE" or
            key == U"ARTIST" or
            key == U"GENRE" or
            key == U"DESIGNER" or
            key == U"DIFFICULTY" or
            key == U"PLAYLEVEL" or
            key == U"SONGIG" or
            key == U"WAVE" or
            key == U"WAVEOFFSET" or
            key == U"JACKET" or
            key == U"BACKGROUND" or
            key == U"MOVIE" or
            key == U"MOVIEOFFSET" or
            key == U"BASEBPM"
        ) {
            return; // Ignore these keys
        }

        if (key == U"REQUEST") {
            AnalyzeRequest(data, ParseRawString(value));
        } else if (key == U"ATTRIBUTE") {
        } else if (key == U"NOATTRIBUTE") {
        } else if (key == U"HISPEED") {
        } else if (key == U"NOSPEED") {
        } else if (key == U"MEASUREBS") {
        } else if (key == U"MEASUREHS") {
        } else {
            // Error: Unknown
            return;
        }
    }

    void AnalyzeRequest(SUSData& data, const s3d::String& cmd) {
        const auto separator = cmd.indexOf(U' ');
        if (separator == s3d::String::npos) {
            // Error: Invalid SUS request command.
            return;
        }

        const auto request = cmd.substr(0, separator).trimmed();
        const auto arg = cmd.substr(separator + 1).trimmed();

        // TODO: Handle error
        if (request == U"ticks_per_beat") {
            data.ticks_per_beat = s3d::ParseInt<uint32>(arg);
        } else if (request == U"enable_priority") {
            // TBD
        } else if (request == U"metronome") {
            // TBD
        }
    }
}
