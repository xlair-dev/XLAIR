#pragma once
#include "SUSAnalyzer/SUSData.hpp"

namespace SheetsAnalyzer::SUSAnalyzer::internal {
    void AnalyzeCommandLine(SUSData& data, const s3d::String& line);
}
