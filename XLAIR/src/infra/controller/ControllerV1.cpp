#include "ControllerV1.hpp"

namespace infra::controller {
    ControllerV1::ControllerV1(StringView port, int32 baudrate)
        : m_port(port), m_baudrate(baudrate) {
    }
    ControllerV1::~ControllerV1() {
        close();
    }

    bool ControllerV1::open() {
        close();
        return m_serial.open(m_port, m_baudrate);
    }

    void ControllerV1::close() {
        if (m_serial.isOpen()) {
            m_serial.close();
        }
        m_initialized = false;
        m_t = 0;
    }

    bool ControllerV1::initialize(uint32 timeout_ms) {
        if (not m_serial.isOpen()) {
            if (not m_serial.open(m_port, m_baudrate)) {
                return false;
            }
        }
        m_initialized = true;
        return true;
    }

    void ControllerV1::update() {
        if (not m_serial.isOpen()) {
            return;
        }

        if (const size_t available = m_serial.available()) {
            const auto line = m_serial.readBytes();

            for (const auto& c : line) {
                if (c == '\n') {
                    // separator
                    if (m_t >= 32) {
                        for (size_t i = 0; i < 32; ++i) {
                            if (m_slider_state[i]) {
                                m_slider_pressed_frames[i]++;
                            } else {
                                m_slider_pressed_frames[i] = 0;
                            }
                        }
                    }
                    m_t = 0;
                } else if (c == '0') {
                    // not pressed
                    if (m_t >= 32) {
                        continue;
                    }
                    m_slider_state[m_t] = false;
                    m_t++;
                } else if (c == '1') {
                    // pressed
                    if (m_t >= 32) {
                        continue;
                    }
                    m_slider_state[m_t] = true;
                    m_t++;
                } else {
                    // invalid
                }
            }
        }
    }

    bool ControllerV1::setLED(const Array<Color>&, uint8) {
        // Not supported
        return false;
    }

    uint32 ControllerV1::sliderPressedFrameCount(size_t index) {
        if (index >= 32) {
            return 0;
        }
        return m_slider_pressed_frames[index];
    }

    uint32 ControllerV1::sidePressedFrameCount(SideButton) {
        // Not supported
        return 0;
    }

    uint64 ControllerV1::lastInputTimestampMs() const {
        // Not supported
        return 0;
    }
}
