#pragma once

#include "app/controller/Types.hpp"

namespace xlair::app::interfaces {
    class IControllerDevice {
    public:
        virtual ~IControllerDevice() = default;

        [[nodiscard]]
        virtual controller::OperationResult initialize() = 0;

        [[nodiscard]]
        virtual controller::OperationResult update() = 0;

        [[nodiscard]]
        virtual controller::OperationResult setLights(const controller::LightFrame& lights) = 0;

        [[nodiscard]]
        virtual const controller::InputSnapshot& snapshot() const noexcept = 0;
    };
}
