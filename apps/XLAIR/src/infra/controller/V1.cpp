#include "V1.hpp"

namespace xlair::infra::controller {
    V1::V1(const StringView port, const int32 baud_rate) : m_port{ port }, m_baud_rate{ baud_rate } {}

    V1::~V1() {
        close();
    }

    app::controller::OperationResult V1::initialize() {
        close();
        m_snapshot = {};
        m_input_frame = {};
        m_input_frame_size = 0;
        m_input_frame_invalid = false;

        if (!m_serial.open(m_port, m_baud_rate)) {
            return app::controller::OperationResult::makeError(
                app::controller::ErrorKind::Connection,
                U"Failed to open the V1 controller serial port '{}'."_fmt(m_port)
            );
        }

        auto keyboard_result = m_keyboard.initialize();
        if (!keyboard_result) {
            close();
            return keyboard_result;
        }

        m_initialized = true;
        return {};
    }

    app::controller::OperationResult V1::update() {
        if (!m_initialized || !m_serial.isOpen()) {
            return app::controller::OperationResult::makeError(
                app::controller::ErrorKind::Connection,
                U"V1 controller is not connected."
            );
        }

        auto keyboard_result = m_keyboard.update();
        if (!keyboard_result) {
            return keyboard_result;
        }
        const auto& keyboard_snapshot = m_keyboard.snapshot();
        m_snapshot.side_buttons = keyboard_snapshot.side_buttons;
        m_snapshot.maintenance_buttons = keyboard_snapshot.maintenance_buttons;

        if (m_serial.available() > 0) {
            for (const auto byte : m_serial.readBytes()) {
                consumeByte(byte);
            }
        }

        return {};
    }

    app::controller::OperationResult V1::setLights(const app::controller::LightFrame&) {
        // V1 has no controllable lights.
        return {};
    }

    const app::controller::InputSnapshot& V1::snapshot() const noexcept {
        return m_snapshot;
    }

    void V1::close() {
        if (m_serial.isOpen()) {
            m_serial.close();
        }

        m_initialized = false;
        m_input_frame_invalid = false;
        m_input_frame_size = 0;
    }

    void V1::consumeByte(const uint8 byte) {
        if (byte == '\r') {
            return;
        }
        if (byte == '\n') {
            finishInputFrame();
            return;
        }
        if (byte != '0' && byte != '1') {
            m_input_frame_invalid = true;
            return;
        }
        if (m_input_frame_size >= m_input_frame.size()) {
            m_input_frame_invalid = true;
            return;
        }

        m_input_frame[m_input_frame_size] = (byte == '1');
        ++m_input_frame_size;
    }

    void V1::finishInputFrame() {
        if (!m_input_frame_invalid && m_input_frame_size == m_input_frame.size()) {
            m_snapshot.touch_zones = m_input_frame;
        }

        m_input_frame = {};
        m_input_frame_size = 0;
        m_input_frame_invalid = false;
    }
}
