#pragma once
#include "app/interfaces/ISheetProvider.hpp"

namespace infra::sheet {
    class SheetProd : public app::interfaces::ISheetProvider {
    public:
        SheetProd() = default;
        ~SheetProd() = default;

        void load(Array<core::types::SheetMetadata>& metadata) override;
        void loadData(core::types::SheetData& data, const FilePath& src, int64 sample_rate, double offset_sec) override;

    private:
        const FilePath Directory = U"data/sheets";
    };
}
