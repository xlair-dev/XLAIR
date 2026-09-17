#pragma once

#include "app/interfaces/ICardReader.hpp"

namespace xlair::infra::card {
    class PasoriRCS3xx final : public app::interfaces::ICardReader {
    public:
        [[nodiscard]]
        app::card::ScanRequest scan() override;
    };
}
