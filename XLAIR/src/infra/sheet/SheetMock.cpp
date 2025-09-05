#include "SheetMock.hpp"

namespace infra::sheet {
    SheetMock::SheetMock(size_t size) : m_size(size) {}

    void SheetMock::load(Array<core::types::SheetMetadata>& metadata) {
        metadata.clear();
        for (size_t i = 0; i < m_size; ++i) {
            core::types::SheetMetadata meta;
            // TODO: 適当なデータを入れる
            metadata.push_back(meta);
        }
    }
}
