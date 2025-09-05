#pragma once
#include "app/interfaces/ISheetProvider.hpp"

namespace infra::sheet {
    class SheetMock : public app::interfaces::ISheetProvider {
    public:
        SheetMock(size_t size);
        ~SheetMock() = default;

        void load(Array<core::types::SheetMetadata>& metadata) override;

    private:
        size_t m_size = 0;
    };
}
