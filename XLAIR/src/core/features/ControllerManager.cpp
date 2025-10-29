#include "ControllerManager.hpp"

namespace core::features {
    ControllerManager::ControllerManager(Device device)
        : m_device(std::move(device)) {
    }

    ControllerManager::~ControllerManager() {
        m_device->close();
    }

    bool ControllerManager::InitializeDevice(uint32 timeout_ms) {
        if (auto p = Addon::GetAddon<ControllerManager>(Name)) {
            return p->initializeDevice(timeout_ms);
        }
        return false;
    }

    const types::DeviceInput ControllerManager::Slider(size_t index) {
        if (auto p = Addon::GetAddon<ControllerManager>(Name)) {
            return p->slider(index);
        }
        return DeviceInput{ 0 };
    }

    const types::DeviceInput ControllerManager::Side(SideButton button) {
        if (auto p = Addon::GetAddon<ControllerManager>(Name)) {
            return p->side(button);
        }
        return DeviceInput{ 0 };
    }

    bool ControllerManager::init() {
        return m_device->open();
    }

    bool ControllerManager::update() {
        if (m_initialized) {
            m_device->update();
        }
        return true;
    }

    bool ControllerManager::initializeDevice(uint32 timeout_ms) {
        m_initialized = (m_device->initialize(timeout_ms));
        return m_initialized;
    }

    const types::DeviceInput ControllerManager::slider(size_t index) {
        return DeviceInput{ m_device->sliderPressedFrameCount(index) };
    }

    const types::DeviceInput ControllerManager::side(SideButton button) {
        return DeviceInput{ m_device->sidePressedFrameCount(button) };
    }
}
