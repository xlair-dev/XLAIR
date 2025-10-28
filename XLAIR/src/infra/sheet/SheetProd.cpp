#include "SheetProd.hpp"
#include "SheetsAnalyzer.hpp"

namespace infra::sheet {
    void SheetProd::load(Array<core::types::SheetMetadata>& metadata) {
        metadata.clear();
        metadata = std::move(SheetsAnalyzer::AnalyzeAll(Directory));
    }

    void SheetProd::loadData(core::types::SheetData& data, const FilePath& src, int64 sample_rate, double offset_sec) {
        data = *SheetsAnalyzer::AnalyzeData(src, sample_rate, offset_sec); // TODO: error handling
    }
}
