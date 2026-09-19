#pragma once

#include "app/interfaces/IControllerDevice.hpp"

namespace xlair::infra::controller {
    class Keyboard final : public app::interfaces::IControllerDevice {
    public:
        [[nodiscard]]
        app::controller::OperationResult initialize() override;

        [[nodiscard]]
        app::controller::OperationResult update() override;

        [[nodiscard]]
        app::controller::OperationResult setLights(const app::controller::LightFrame&) override;

        [[nodiscard]]
        const app::controller::InputSnapshot& snapshot() const noexcept override;

    private:
        app::controller::InputSnapshot m_snapshot;
    };
}
