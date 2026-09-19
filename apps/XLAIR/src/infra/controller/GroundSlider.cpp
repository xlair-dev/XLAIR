#include "GroundSlider.hpp"

#include <Siv3D/System.hpp>
#include <Siv3D/Time.hpp>

#include <algorithm>
#include <span>

namespace xlair::infra::controller {
    namespace {
        constexpr uint32 InitializationTimeoutMs = 1'000;
        constexpr uint8 ResetCommand = 0x10;
        constexpr uint8 HardwareInfoCommand = 0xF0;
        constexpr uint8 TouchInputCommand = 0x01;
        constexpr uint8 SetLightsCommand = 0x02;
        constexpr uint8 StartInputCommand = 0x03;
        constexpr uint8 StopInputCommand = 0x04;
        constexpr uint8 MaximumComponent = 0xFC;
        constexpr uint8 MaximumBrightness = 0x3F;

        [[nodiscard]]
        uint8 Checksum(const std::span<const uint8> bytes) {
            uint32 sum = 0;
            for (const auto byte : bytes) {
                sum += byte;
            }
            return static_cast<uint8>((~sum + 1) & 0xFF);
        }

    }

    GroundSlider::GroundSlider(const StringView port, const int32 baud_rate, const uint8 touch_threshold)
        : m_port{ port }, m_baud_rate{ baud_rate }, m_touch_threshold{ touch_threshold } {}

    GroundSlider::~GroundSlider() {
        close();
    }

    app::controller::OperationResult GroundSlider::initialize() {
        close();
        m_snapshot = {};
        m_slider_lights = {};
        m_lights_sent = false;

        if (!m_serial.open(m_port, m_baud_rate)) {
            return app::controller::OperationResult::makeError(
                app::controller::ErrorKind::Connection,
                U"Failed to open the GroundSlider serial port '{}'."_fmt(m_port)
            );
        }

        if (!sendRawCommand({ 0xFF, ResetCommand, 0x00 }) || !consumeUntilOnePacket(InitializationTimeoutMs) ||
            m_last_packet.command != ResetCommand || !m_last_packet.payload.isEmpty()) {
            close();
            return app::controller::OperationResult::makeError(
                app::controller::ErrorKind::Communication,
                U"GroundSlider did not acknowledge reset."
            );
        }

        if (!sendRawCommand({ 0xFF, HardwareInfoCommand, 0x00 }) || !consumeUntilOnePacket(InitializationTimeoutMs) ||
            m_last_packet.command != HardwareInfoCommand || m_last_packet.payload.size() < 18) {
            close();
            return app::controller::OperationResult::makeError(
                app::controller::ErrorKind::Communication,
                U"GroundSlider returned invalid hardware information."
            );
        }

        if (!startInput()) {
            close();
            return app::controller::OperationResult::makeError(
                app::controller::ErrorKind::Communication,
                U"Failed to start GroundSlider input."
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

    app::controller::OperationResult GroundSlider::update() {
        if (!m_initialized || !m_serial.isOpen()) {
            return app::controller::OperationResult::makeError(
                app::controller::ErrorKind::Connection,
                U"GroundSlider is not connected."
            );
        }

        auto keyboard_result = m_keyboard.update();
        if (!keyboard_result) {
            return keyboard_result;
        }
        const auto& keyboard_snapshot = m_keyboard.snapshot();
        m_snapshot.side_buttons = keyboard_snapshot.side_buttons;
        m_snapshot.maintenance_buttons = keyboard_snapshot.maintenance_buttons;

        pumpRx();
        Optional<std::array<uint8, app::controller::TouchZoneCount>> latest_touch_frame;
        while (consumeOnePacket()) {
            if (m_last_packet.command != TouchInputCommand ||
                m_last_packet.payload.size() != app::controller::TouchZoneCount) {
                continue;
            }

            std::array<uint8, app::controller::TouchZoneCount> zones;
            std::copy(m_last_packet.payload.begin(), m_last_packet.payload.end(), zones.begin());
            latest_touch_frame = zones;
        }

        if (latest_touch_frame) {
            for (std::size_t index = 0; index < latest_touch_frame->size(); ++index) {
                const auto destination = app::controller::TouchZoneCount - 1 - index;
                m_snapshot.touch_zones[destination] = ((*latest_touch_frame)[index] >= m_touch_threshold);
            }

            // Keep the lights active. GroundSlider may turn them off when updates stop.
            if (m_lights_sent && !sendLights(m_slider_lights)) {
                return app::controller::OperationResult::makeError(
                    app::controller::ErrorKind::Communication,
                    U"Failed to refresh GroundSlider lights."
                );
            }
        }

        return {};
    }

    app::controller::OperationResult GroundSlider::setLights(const app::controller::LightFrame& lights) {
        if (!m_initialized || !m_serial.isOpen()) {
            return app::controller::OperationResult::makeError(
                app::controller::ErrorKind::Connection,
                U"GroundSlider is not connected."
            );
        }

        if (m_lights_sent && m_slider_lights == lights.slider) {
            return {};
        }
        if (!sendLights(lights.slider)) {
            return app::controller::OperationResult::makeError(
                app::controller::ErrorKind::Communication,
                U"Failed to send GroundSlider lights."
            );
        }

        m_slider_lights = lights.slider;
        m_lights_sent = true;
        return {};
    }

    const app::controller::InputSnapshot& GroundSlider::snapshot() const noexcept {
        return m_snapshot;
    }

    void GroundSlider::close() {
        if (m_serial.isOpen()) {
            if (m_input_enabled) {
                stopInput();
            }
            m_serial.close();
        }

        m_initialized = false;
        m_input_enabled = false;
        m_lights_sent = false;
        m_receive_buffer.clear();
        m_last_packet = {};
    }

    bool GroundSlider::startInput() {
        m_input_enabled = m_serial.isOpen() && sendRawCommand({ 0xFF, StartInputCommand, 0x00 });
        return m_input_enabled;
    }

    bool GroundSlider::stopInput() {
        m_input_enabled = false;
        return m_serial.isOpen() && sendRawCommand({ 0xFF, StopInputCommand, 0x00 });
    }

    bool GroundSlider::sendLights(const std::array<Color, app::controller::SliderLightCount>& lights) {
        if (!m_serial.isOpen()) {
            return false;
        }

        Array<uint8> body;
        body.reserve(4 + (app::controller::SliderLightCount * 3));
        body << 0xFF << SetLightsCommand << static_cast<uint8>(1 + (app::controller::SliderLightCount * 3))
             << MaximumBrightness;
        for (const auto& color : lights) {
            const auto clamp = [](const uint8 component) {
                return std::min(component, MaximumComponent);
            };
            body << clamp(color.b) << clamp(color.r) << clamp(color.g);
        }
        return sendRawCommand(body);
    }

    bool GroundSlider::sendRawCommand(const Array<uint8>& body) {
        if (!m_serial.isOpen() || body.isEmpty()) {
            return false;
        }

        const auto checksum = Checksum(std::span<const uint8>{ body.data(), body.size() });
        return m_serial.write(body.data(), body.size()) == body.size() && m_serial.writeByte(checksum);
    }

    void GroundSlider::pumpRx() {
        if (m_serial.available() > 0) {
            m_receive_buffer.append(m_serial.readBytes());
        }
    }

    bool GroundSlider::consumeOnePacket() {
        while (true) {
            const auto sync = std::find(m_receive_buffer.begin(), m_receive_buffer.end(), uint8{ 0xFF });
            if (sync == m_receive_buffer.end()) {
                m_receive_buffer.clear();
                return false;
            }
            if (sync != m_receive_buffer.begin()) {
                m_receive_buffer.erase(m_receive_buffer.begin(), sync);
            }

            if (m_receive_buffer.size() < 3) {
                return false;
            }

            const auto payload_size = static_cast<std::size_t>(m_receive_buffer[2]);
            const auto packet_size = 3 + payload_size + 1;
            if (m_receive_buffer.size() < packet_size) {
                return false;
            }

            const auto body = std::span<const uint8>{ m_receive_buffer.data(), packet_size - 1 };
            if (Checksum(body) != m_receive_buffer[packet_size - 1]) {
                m_receive_buffer.erase(m_receive_buffer.begin());
                continue;
            }

            m_last_packet.command = m_receive_buffer[1];
            m_last_packet.payload = Array<uint8>{
                m_receive_buffer.begin() + 3,
                m_receive_buffer.begin() + 3 + payload_size,
            };
            m_receive_buffer.erase(m_receive_buffer.begin(), m_receive_buffer.begin() + packet_size);
            return true;
        }
    }

    bool GroundSlider::consumeUntilOnePacket(const uint32 timeout_ms) {
        const auto started_at = Time::GetMillisec();
        while ((Time::GetMillisec() - started_at) < timeout_ms) {
            pumpRx();
            if (consumeOnePacket()) {
                return true;
            }
            System::Sleep(1);
        }
        return false;
    }
}
