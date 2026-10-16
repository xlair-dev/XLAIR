#include "V1.hpp"

#include <Siv3D.hpp>

#include <algorithm>

namespace xlair::infra::controller {
    namespace {
        using app::controller::InputStatus;

        [[nodiscard]]
        InputStatus NextStatus(const bool available, const InputStatus previous) {
            if (!available) {
                return InputStatus::Unavailable;
            }
            return previous == InputStatus::Unavailable ? InputStatus::Resynchronized : InputStatus::Available;
        }

        constexpr std::array<app::controller::SideButton, 4> SideButtons{
            app::controller::SideButton::LeftUpper,
            app::controller::SideButton::LeftLower,
            app::controller::SideButton::RightUpper,
            app::controller::SideButton::RightLower,
        };
        constexpr std::size_t HardwareMaintenanceButtonCount = 3;
        static_assert(V1Input::SensorCount == app::controller::TouchZoneCount);
    }

    V1::V1(const StringView port, const int32 baud_rate, const uint8 touch_threshold)
        : m_port{ port }, m_baud_rate{ baud_rate }, m_touch_threshold{ touch_threshold } {}

    V1::~V1() {
        close();
    }

    app::controller::OperationResult V1::initialize() {
        close();
        m_snapshot = {};
        m_snapshot.touch_zone_status.fill(InputStatus::Unavailable);
        m_snapshot.side_button_status.fill(InputStatus::Unavailable);
        for (std::size_t index = 0; index < HardwareMaintenanceButtonCount; ++index) {
            m_snapshot.maintenance_button_status[index] = InputStatus::Unavailable;
        }

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

        m_input.reset(Time::GetMillisec());
        m_initialized = true;
        return {};
    }

    app::controller::OperationResult V1::update() {
        if (!m_initialized) {
            return app::controller::OperationResult::makeError(
                app::controller::ErrorKind::Connection,
                U"V1 controller has not been initialized."
            );
        }

        auto keyboard_result = m_keyboard.update();
        if (!keyboard_result) {
            return keyboard_result;
        }
        const auto now = Time::GetMillisec();
        if (m_serial.isOpen() && m_serial.available() > 0) {
            for (const auto byte : m_serial.readBytes()) {
                m_input.consumeByte(byte, now);
            }
        }

        if (m_input.streamTimedOut(now)) {
            // A USB CDC handle may still look open after unplugging.
            // Retry the configured port once per second until frames resume.
            m_serial.close();
            m_input.reset(now);
            m_serial.open(m_port, m_baud_rate);
        }

        const auto& sensors = m_input.sensors();
        for (std::size_t index = 0; index < sensors.size(); ++index) {
            const bool available = m_input.boardAvailable(index / V1Input::SensorsPerBoard, now);
            m_snapshot.touch_zones[index] = available && sensors[index] >= m_touch_threshold;
            m_snapshot.touch_zone_status[index] = NextStatus(available, m_snapshot.touch_zone_status[index]);
        }

        // Keep keyboard buttons available while physical buttons are not connected.
        const auto& keyboard_snapshot = m_keyboard.snapshot();
        m_snapshot.side_buttons = keyboard_snapshot.side_buttons;
        m_snapshot.maintenance_buttons = keyboard_snapshot.maintenance_buttons;
        const auto buttons = m_input.buttons();
        const bool buttons_available = m_input.buttonsAvailable(now);
        for (std::size_t index = 0; index < SideButtons.size(); ++index) {
            const auto destination = static_cast<std::size_t>(SideButtons[index]);
            m_snapshot.side_buttons[destination] |= buttons_available && (buttons & (1u << index)) != 0;
            // A keyboard press is a fresh observation even while hardware is missing.
            m_snapshot.side_button_status[destination] =
                keyboard_snapshot.side_buttons[destination]
                    ? InputStatus::Available
                    : NextStatus(buttons_available, m_snapshot.side_button_status[destination]);
        }
        for (std::size_t index = 0; index < HardwareMaintenanceButtonCount; ++index) {
            m_snapshot.maintenance_buttons[index] |= buttons_available && (buttons & (1u << (4 + index))) != 0;
            m_snapshot.maintenance_button_status[index] =
                keyboard_snapshot.maintenance_buttons[index]
                    ? InputStatus::Available
                    : NextStatus(buttons_available, m_snapshot.maintenance_button_status[index]);
        }

        return {};
    }

    app::controller::OperationResult V1::setLights(const app::controller::LightFrame&) {
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
        m_input.reset(0);
    }

    void V1Input::reset(const std::uint64_t now_ms) {
        m_frame = {};
        m_frame_size = 0;
        m_sensors = {};
        m_buttons = 0;
        m_last_valid_at = {};
        m_last_frame_at.reset();
        m_started_at = now_ms;
    }

    void V1Input::consumeByte(const std::uint8_t byte, const std::uint64_t now_ms) {
        // SYNC
        if (byte == 0xFF) {
            m_frame[0] = byte;
            m_frame_size = 1;
            return;
        }
        if (m_frame_size == 0) {
            return;
        }

        m_frame[m_frame_size++] = byte;
        if (m_frame_size == m_frame.size()) {
            finishFrame(now_ms);
            m_frame_size = 0;
        }
    }

    const std::array<std::uint8_t, V1Input::SensorCount>& V1Input::sensors() const noexcept {
        return m_sensors;
    }

    std::uint8_t V1Input::buttons() const noexcept {
        return m_buttons;
    }

    bool V1Input::boardAvailable(const std::size_t board, const std::uint64_t now_ms) const noexcept {
        if (board >= BoardCount) {
            return false;
        }
        const auto& last_valid = m_last_valid_at[board];
        return last_valid && now_ms - *last_valid < ValueHoldMs;
    }

    bool V1Input::buttonsAvailable(const std::uint64_t now_ms) const noexcept {
        return m_last_frame_at && now_ms - *m_last_frame_at < ValueHoldMs;
    }

    bool V1Input::streamTimedOut(const std::uint64_t now_ms) const noexcept {
        return now_ms - m_last_frame_at.value_or(m_started_at) >= StreamTimeoutMs;
    }

    void V1Input::finishFrame(const std::uint64_t now_ms) {
        const auto valid = m_frame[2];
        const auto buttons = m_frame[3];
        if ((valid & 0xF0) != 0 || (buttons & 0x80) != 0) {
            return;
        }

        m_buttons = buttons;
        m_last_frame_at = now_ms;
        for (std::size_t board = 0; board < BoardCount; ++board) {
            if ((valid & (1u << board)) == 0) {
                continue;
            }

            const auto offset = board * SensorsPerBoard;
            std::copy_n(m_frame.begin() + 4 + offset, SensorsPerBoard, m_sensors.begin() + offset);
            m_last_valid_at[board] = now_ms;
        }
    }
}
