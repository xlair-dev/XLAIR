#include "ControllerManager.hpp"

namespace core::features {
    ControllerManager::ControllerManager(Device device, double latency_offset)
        : m_device(std::move(device))
        , m_latency_offset(latency_offset) {
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

    std::pair<uint32, uint32> ControllerManager::SliderTouchFrames(size_t index) {
        if (auto p = Addon::GetAddon<ControllerManager>(Name)) {
            return p->sliderTouchFrames(index);
        }
        return { 0, 0 };
    }

    uint64 ControllerManager::GetLastInputTimestampMs() {
        if (auto p = Addon::GetAddon<ControllerManager>(Name)) {
            return p->getLastInputTimestampMs();
        }
        return 0;
    }

    double ControllerManager::LatencyOffset() {
        if (auto p = Addon::GetAddon<ControllerManager>(Name)) {
            return p->m_latency_offset;
        }
        return 0.0;
    }

    bool ControllerManager::init() {
        return m_device->open();
    }

    bool ControllerManager::update() {
        if (m_initialized) {
            for (size_t i = 0; i < 32; ++i) {
                m_prev_touch_frame_count[i] = m_device->sliderPressedFrameCount(i);
            }
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

    std::pair<uint32, uint32> ControllerManager::sliderTouchFrames(size_t index) {
        return { m_prev_touch_frame_count[index], m_device->sliderPressedFrameCount(index) };
    }

    uint64 ControllerManager::getLastInputTimestampMs() const {
        return m_device->lastInputTimestampMs();
    }
}
