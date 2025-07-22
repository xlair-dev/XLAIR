#pragma once
#include "SUSAnalyzer/SUSAnalyzer.hpp"

namespace SheetsAnalyzer::SUSAnalyzer::internal {
    bool IsCommandLine(const s3d::String& line);
    bool IsDataLine(const s3d::String& line);
}
