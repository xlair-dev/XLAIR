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
}
