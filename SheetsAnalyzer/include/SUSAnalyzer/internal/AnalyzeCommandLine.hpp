#pragma once
#include "SUSAnalyzer/SuSData.hpp"

namespace SheetsAnalyzer::SUSAnalyzer::internal {
    void AnalyzeCommandLine(SUSData& data, const s3d::String& line);

    void AnalyzeRequest(SUSData& data, const s3d::String& cmd);
}
