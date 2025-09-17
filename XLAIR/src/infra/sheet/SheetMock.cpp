#include "SheetMock.hpp"
#include "SheetsAnalyzer.hpp"

namespace infra::sheet {
    SheetMock::SheetMock(size_t size) : m_size(size) {}

    void SheetMock::load(Array<core::types::SheetMetadata>& metadata) {
        metadata.clear();
        const auto mock_meta = loadMockMetadata();
        for (size_t i = 0; i < m_size; ++i) {
            metadata.push_back(mock_meta);
        }
    }

    core::types::SheetMetadata SheetMock::loadMockMetadata() const {
        return SheetsAnalyzer::Analyze(MockDataPath).value_or({});
    }
}
