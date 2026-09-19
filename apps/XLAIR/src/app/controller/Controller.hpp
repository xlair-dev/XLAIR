#pragma once

#include "ButtonState.hpp"
#include "app/interfaces/IControllerDevice.hpp"

#include <array>
#include <memory>

namespace xlair::app::controller {
    class Controller {
    public:
        explicit Controller(std::unique_ptr<interfaces::IControllerDevice> device);

        [[nodiscard]]
        OperationResult initialize();

        [[nodiscard]]
        OperationResult update();

        [[nodiscard]]
        OperationResult setLights(const LightFrame& lights);

        [[nodiscard]]
        const LightFrame& lights() const noexcept;

        // +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
        // |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |
        // |  0 |  2 |  4 |  6 |  8 | 10 | 12 | 14 | 16 | 18 | 20 | 22 | 24 | 26 | 28 | 30 |
        // |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |
        // +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
        // |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |
        // |  1 |  3 |  5 |  7 |  9 | 11 | 13 | 15 | 17 | 19 | 21 | 23 | 25 | 27 | 29 | 31 |
        // |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |
        // +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
        [[nodiscard]]
        const ButtonState& touchZone(std::size_t index) const;

        [[nodiscard]]
        const ButtonState& sideButton(SideButton button) const noexcept;

        [[nodiscard]]
        const ButtonState& maintenanceButton(MaintenanceButton button) const noexcept;

    private:
        std::unique_ptr<interfaces::IControllerDevice> m_device;
        std::array<ButtonState, TouchZoneCount> m_touch_zones;
        std::array<ButtonState, SideButtonCount> m_side_buttons;
        std::array<ButtonState, MaintenanceButtonCount> m_maintenance_buttons;
        LightFrame m_lights;
        bool m_initialized = false;
    };
}
