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

        [[nodiscard]]
        Array<String> FindV1Ports() {
            Array<String> candidates;
            Array<String> identified;
            for (const auto& info : System::EnumerateSerialPorts()) {
                const auto hardware_id = info.hardwareID.lowercased();
                const auto hardware_fields = hardware_id.split(U' ');
                // Siv3D returns different hardware ID formats on Windows and Unix.
                const bool matches_usb_id =
                    hardware_fields.contains(U"vid:pid=2e8a:000a") || hardware_id.contains(U"vid_2e8a&pid_000a");
                if (!matches_usb_id) {
                    continue;
                }

                const auto description_fields = info.description.lowercased().split(U' ');
                // The setup firmware shares the VID/PID but is not an input device.
                if (description_fields.contains(U"xlair-controller_v1_setup") ||
                    hardware_fields.any([](const String& field) {
                        return field.starts_with(U"snr=") && field != U"snr=none" && field != U"snr=controller-v1";
                    })) {
                    continue;
                }

                candidates.push_back(info.port);
                if (description_fields.contains(U"xlair-controller_v1") ||
                    hardware_fields.contains(U"snr=controller-v1")) {
                    identified.push_back(info.port);
                }
            }

            // Windows may expose only VID/PID and a generic USB serial description.
            return identified.isEmpty() ? candidates : identified;
        }
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

        if (auto result = openPort(); !result) {
            return result;
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
        if (m_serial.isOpen()) {
            if (const auto available = m_serial.available(); available > 0) {
                Array<uint8> bytes(available);
                const auto read_size = m_serial.read(bytes.data(), bytes.size_bytes());
                bytes.resize(read_size);
                for (const auto byte : bytes) {
                    m_input.consumeByte(byte, now);
                }
            }
        }

        if (m_last_reconnect_error && m_input.buttonsAvailable(now)) {
            Logger << U"[Controller/V1] Frame reception resumed on '{}'."_fmt(m_serial.port());
            m_last_reconnect_error.reset();
        }

        if (m_input.streamTimedOut(now)) {
            // A USB CDC handle may still look open after unplugging.
            // Retry once per second, resolving the port again in auto mode.
            m_serial.close();
            m_input.reset(now);
            if (auto result = openPort(); !result) {
                const auto& message = result.error->message;
                if (!m_last_reconnect_error || *m_last_reconnect_error != message) {
                    Logger << U"[Controller/V1] Reconnect failed: " + message;
                    m_last_reconnect_error = message;
                }
            }
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

    app::controller::OperationResult V1::openPort() {
        String port = m_port;
        if (port.lowercased() == U"auto") {
            const auto candidates = FindV1Ports();
            if (candidates.isEmpty()) {
                return app::controller::OperationResult::makeError(
                    app::controller::ErrorKind::Connection,
                    U"No V1 controller was found (USB VID 2E8A, PID 000A). "
                    U"If USB identifiers are unavailable, set 'controller.v1.port' to an explicit port name."
                );
            }
            if (candidates.size() > 1) {
                return app::controller::OperationResult::makeError(
                    app::controller::ErrorKind::Connection,
                    U"Multiple V1 controller candidates were found: {}. Set 'controller.v1.port' explicitly."_fmt(
                        candidates.join(U", ", U"", U"")
                    )
                );
            }
            port = candidates.front();
        }

        if (!m_serial.open(port, m_baud_rate)) {
            return app::controller::OperationResult::makeError(
                app::controller::ErrorKind::Connection,
                U"Failed to open the V1 controller serial port '{}'."_fmt(port)
            );
        }
        return {};
    }

    void V1::close() {
        if (m_serial.isOpen()) {
            m_serial.close();
        }

        m_initialized = false;
        m_input.reset(0);
        m_last_reconnect_error.reset();
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
