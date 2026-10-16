#pragma once

#include <chart_converter.h>
#include <memory>
#include <string_view>

namespace fixtures {
    using ConverterResult = std::unique_ptr<CcResult, decltype(&cc_result_free)>;

    [[nodiscard]]
    inline ConverterResult ParseChart(const std::string_view source) {
        return {
            cc_parse(CcFormat_Sus, CcMode_Xlair, reinterpret_cast<const uint8_t*>(source.data()), source.size()),
            cc_result_free,
        };
    }
}
