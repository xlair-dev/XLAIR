#include "Controller.hpp"

#include <utility>

namespace xlair::app::controller {
    Controller::Controller(std::unique_ptr<interfaces::IControllerDevice> device) : m_device{ std::move(device) } {}

    OperationResult Controller::initialize() {
        m_touch_zones = {};
        m_side_buttons = {};
        m_maintenance_buttons = {};
        m_lights = {};
        m_initialized = false;

        if (!m_device) {
            return OperationResult{ Error{
                .kind = ErrorKind::Unavailable,
                .message = U"A controller device is not available.",
            } };
        }

        auto result = m_device->initialize();
        m_initialized = static_cast<bool>(result);
        if (m_initialized) {
            result = m_device->setLights(m_lights);
            m_initialized = static_cast<bool>(result);
        }
        return result;
    }

    OperationResult Controller::update() {
        if (!m_initialized) {
            return OperationResult{
                Error{
                    .kind = ErrorKind::Unavailable,
                    .message = U"The controller has not been initialized.",
                },
            };
        }

        auto result = m_device->update();
        if (!result) {
            m_touch_zones = {};
            m_side_buttons = {};
            m_maintenance_buttons = {};
            m_initialized = false;
            return result;
        }

        const auto& snapshot = m_device->snapshot();
        for (std::size_t index = 0; index < m_touch_zones.size(); ++index) {
            m_touch_zones[index].update(snapshot.touch_zones[index]);
        }
        for (std::size_t index = 0; index < m_side_buttons.size(); ++index) {
            m_side_buttons[index].update(snapshot.side_buttons[index]);
        }
        for (std::size_t index = 0; index < m_maintenance_buttons.size(); ++index) {
            m_maintenance_buttons[index].update(snapshot.maintenance_buttons[index]);
        }

        return {};
    }

    OperationResult Controller::setLights(const LightFrame& lights) {
        if (!m_initialized) {
            return OperationResult{
                Error{
                    .kind = ErrorKind::Unavailable,
                    .message = U"The controller has not been initialized.",
                },
            };
        }

        auto result = m_device->setLights(lights);
        if (result) {
            m_lights = lights;
        }
        return result;
    }

    const LightFrame& Controller::lights() const noexcept {
        return m_lights;
    }

    const ButtonState& Controller::touchZone(const std::size_t index) const {
        return m_touch_zones.at(index);
    }

    const ButtonState& Controller::sideButton(const SideButton button) const noexcept {
        return m_side_buttons[static_cast<std::size_t>(button)];
    }

    const ButtonState& Controller::maintenanceButton(const MaintenanceButton button) const noexcept {
        return m_maintenance_buttons[static_cast<std::size_t>(button)];
    }
}
