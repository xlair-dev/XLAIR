#pragma once
#include "SUSAnalyzer/SuSData.hpp"

namespace SheetsAnalyzer::SUSAnalyzer::internal {
    void AnalyzeDataLine(SUSData& data, const s3d::String& line);
}

#include "SUSAnalyzer/internal/AnalyzeNotes.ipp"
