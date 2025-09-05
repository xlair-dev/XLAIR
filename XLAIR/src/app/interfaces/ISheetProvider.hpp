#pragma once
#include "Common.hpp"
#include "core/types/Sheet.hpp"

namespace app::interfaces {
    class ISheetProvider {
    public:
        virtual ~ISheetProvider() = default;
        virtual void load(Array<core::types::SheetMetadata>& metadata) = 0;
        virtual void loadData(core::types::SheetData& data) = 0;
    };
}
