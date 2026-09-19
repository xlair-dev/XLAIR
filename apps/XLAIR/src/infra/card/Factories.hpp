#pragma once

#include "app/config/Config.hpp"
#include "app/interfaces/ICardReader.hpp"

namespace xlair::infra::card {
    [[nodiscard]]
    std::unique_ptr<app::interfaces::ICardReader> CreateReader(const app::Config::CardReader& config);
}
