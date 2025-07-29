#pragma once
#include "SheetData.hpp"

namespace SheetsAnalyzer::SUSAnalyzer::internal {
    void AnalyzeDataLine(SheetData& data, const s3d::String& line);
}

#include "SUSAnalyzer/internal/AnalyzeNotes.ipp"
