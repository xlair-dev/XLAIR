#pragma once

#include "app/config/Config.hpp"
#include "app/interfaces/IControllerDevice.hpp"

#include <memory>

namespace xlair::infra::controller {
    [[nodiscard]]
    std::unique_ptr<app::interfaces::IControllerDevice> CreateDevice(const app::Config::Controller& config);
}
