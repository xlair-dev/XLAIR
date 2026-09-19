#pragma once

#include "app/card/Types.hpp"

namespace xlair::app::interfaces {
    class ICardReader {
    public:
        virtual ~ICardReader() = default;

        [[nodiscard]]
        virtual card::ScanRequest scan() = 0;
    };
}
