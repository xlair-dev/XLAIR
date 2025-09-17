#pragma once
#include "SUSAnalyzer/SUSAnalyzer.hpp"

namespace SheetsAnalyzer::SUSAnalyzer::internal {
    inline bool IsCommandLine(const s3d::String& line) {
        // Check if the line matches the expected format:
        // ^#[A-Za-z0-9]+(?: +[^\n]+)?$

        size_t i = 0;
        const auto n = line.length();

        // Check if the line starts with '#'
        if (n == 0 or line[i] != '#') {
            return false;
        }
        i++;

        // Check for at least 1 alphanumeric character
        if (i == n or not s3d::IsAlnum(line[i])) {
            return false;
        }
        while (i < n and s3d::IsAlnum(line[i])) {
            i++;
        }

        // No option data
        if (i == 0) {
            return true;
        }

        // Check for at least 1 space
        if (not s3d::IsSpace(line[i])) {
            return false;
        }
        while (i < n and s3d::IsSpace(line[i])) {
            i++;
        }

        // Check if the rest of the line contains valid characters
        if (i == n or line[i] == '\n') {
            return false;
        }
        while (i < n) {
            if (line[i] == '\n') {
                return false;
            }
            i++;
        }

        return true;
    }

    inline bool IsDataLine(const s3d::String& line) {
        // Check if the line matches the expected format:
        // ^#[A-Za-z0-9]{3}[A-Za-z0-9]{2,3}:\s*[^\n]+$

        size_t i = 0;
        const auto n = line.length();

        // Check if the line starts with '#'
        if (n == 0 or line[i] != '#') {
            return false;
        }
        i++;

        // Check for at least 3 alphanumeric characters
        for (int j = 0; j < 3; ++j) {
            if (i >= n or not s3d::IsAlnum(line[i])) {
                return false;
            }
            i++;
        }

        // Check for 2 to 3 additional alphanumeric characters
        for (int j = 0; j < 2; ++j) {
            if (i >= n or not s3d::IsAlnum(line[i])) {
                return false;
            }
            i++;
        }

        // Allow for an optional third character
        if (i < n and s3d::IsAlnum(line[i])) {
            i++;
        }

        // :
        if (i >= n or line[i] != ':') {
            return false;
        }
        i++;

        // Skip whitespace after ':'
        while (i < n and s3d::IsSpace(line[i])) {
            i++;
        }

        // Check if there is at least one character after ':'
        if (i >= n) {
            return false; // No data after ':'
        }
        // Check if the rest of the line contains valid characters
        for (; i < n; ++i) {
            if (not s3d::IsPrint(line[i]) or line[i] == '\n') {
                return false; // Invalid character or newline in data
            }
        }

        // If we reach here, the line is valid
        return true;
    }

    inline bool IsNoteData(s3d::StringView meas) {
        for (const auto& c : meas) {
            if (not s3d::IsDigit(c)) {
                return false;
            }
        }
        return true;
    }

    inline s3d::String ParseRawString(s3d::StringView raw) {
        if (raw.front() == '\"' and raw.back() == '\"') {
            // TODO: process escaped characters
            const auto length = raw.size();
            return s3d::String{ raw.substr(1, length - 2) };
        } else {
            return s3d::String{ raw };
        }
    }

    inline s3d::Optional<SUSHispeedData> ParseHispeedData(s3d::StringView raw) {
        // Expected format: "measure'ticks:speed"
        const auto separator = raw.indexOf(U':');
        if (separator == s3d::String::npos) {
            // Error: Invalid HISPEED format.
            return s3d::none;
        }
        const auto timing = raw.substr(0, separator);
        const auto speed_str = raw.substr(separator + 1);

        const auto timing_separator = timing.indexOf(U'\'');
        if (timing_separator == s3d::String::npos) {
            // Error: Invalid HISPEED timing format.
            return s3d::none;
        }
        SUSRelativeNoteTime time = {
            .measure = s3d::ParseInt<s3d::uint32>(timing.substr(0, timing_separator), 10),
            .ticks = s3d::ParseInt<s3d::uint32>(timing.substr(timing_separator + 1), 10)
        };
        auto speed = s3d::ParseFloat<double>(speed_str);
        if (speed == 0.0) {
            speed = 0.0000001; // Avoid zero speed
        }
        return SUSHispeedData{ time, speed };
    }
}
