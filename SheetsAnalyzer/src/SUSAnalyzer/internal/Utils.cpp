#include "SUSAnalyzer/internal/Utils.hpp"

namespace SheetsAnalyzer::SUSAnalyzer::internal {
    bool IsCommandLine(const s3d::String& line) {
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

    bool IsDataLine(const s3d::String& line) {
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
}
