#include "ControllerGroundSlider.hpp"
#include <span>

namespace infra::controller {
    namespace {
        static uint8 Checksum(std::span<const uint8> payload) {
            uint32 s = 0;
            for (const auto& byte : payload) {
                s += byte;
            }
            return static_cast<uint8>((~s + 1) & 0xFF);
        }
    }

    ControllerGroundSlider::ControllerGroundSlider(StringView port, int32 baudrate, uint8 threshold)
        :m_port(port), m_baudrate(baudrate), m_threshold(threshold) {}

    ControllerGroundSlider::~ControllerGroundSlider() {
        close();
    }

    bool ControllerGroundSlider::open() {
        close();
        return m_serial.open(m_port, m_baudrate);
    }

    void ControllerGroundSlider::close() {
        if (m_serial.isOpen()) {
            if (m_enable_input) {
                stopInput();
            }
            m_serial.close();
        }
        m_initialized = false;
        m_buffer.clear();
        m_hw = {};
        m_sync = false;
        m_enable_input = false;
    }

    bool ControllerGroundSlider::initialize(uint32 timeout_ms) {
        if (not m_serial.isOpen()) {
            if (not m_serial.open(m_port, m_baudrate)) {
                return false;
            }
        }

        m_initialized = false;
        m_buffer.clear();
        m_hw = {};
        m_sync = false;
        m_enable_input = false;
        m_color = Array<Color>(31, Palette::Black);

        sendRawCommand({ 0xFF, 0x10, 0x00 }); // send RESET
        if (not consumeUntilOnePacket(timeout_ms)) {
            return false;
        }
        if (not (m_last_packet.cmd == 0x10 and m_last_packet.len == 0)) {
            return false;
        }

        sendRawCommand({ 0xFF, 0xF0, 0x00 }); // send HW INFO
        if (not consumeUntilOnePacket(timeout_ms)) {
            return false;
        }
        {
            const auto& payload = m_last_packet.payload;
            if (payload.size() < 18) {
                return false;
            }

            m_hw.model = String{ payload.begin(), payload.begin() + 8 };
            m_hw.device_class = payload[8];
            m_hw.chip_part_number = String{ payload.begin() + 9, payload.begin() + 14 };
            m_hw.unk_0xe = payload[14];
            m_hw.firmware_verison = payload[15];
            m_hw.unk_0x10 = payload[16];
            m_hw.unk_0x11 = payload[17];
            m_hw.valid = true;
        }

        m_initialized = m_hw.valid;
        if (m_initialized) {
            startInput();
        }

        return m_initialized;
    }

    void ControllerGroundSlider::update() {
        if (not m_serial.isOpen()) {
            return;
        }

        pumpRx();
        TouchFrame f{};
        while (consumeOnePacket()) {
            if (m_last_packet.cmd == 0x01 and m_last_packet.len == 0x20 and m_last_packet.payload.size() == 32) {
                std::memcpy(f.zones.data(), m_last_packet.payload.data(), 32);
                f.timestamp_ms = Time::GetMillisec();
                f.valid = true;
            }
        }
        if (f.valid) {
            for (size_t i = 0; i < 32; ++i) {
                if (f.zones[i] >= m_threshold) {
                    m_slider_pressed_frames[31 - i]++;
                } else {
                    m_slider_pressed_frames[31 - i] = 0;
                }
            }
            m_last_input_timestamp_ms = f.timestamp_ms;
            sendLED();
        }
    }

    bool ControllerGroundSlider::setLED(const Array<Color>& color, uint8 brightness) {
        if (not m_serial.isOpen() or color.size() != 31) {
            return false;
        }

        m_color = color;
        m_brightness = brightness;
        return true;
    }

    uint32 ControllerGroundSlider::sliderPressedFrameCount(size_t index) {
        return m_slider_pressed_frames[index];
    }

    uint32 ControllerGroundSlider::sidePressedFrameCount(SideButton) {
        return 0;
    }

    uint64 ControllerGroundSlider::lastInputTimestampMs() const {
        return m_last_input_timestamp_ms;
    }

    bool ControllerGroundSlider::startInput() {
        m_enable_input = (m_serial.isOpen() and sendRawCommand({ 0xFF, 0x03, 0x00 }));
        return m_enable_input;
    }

    bool ControllerGroundSlider::stopInput() {
        m_enable_input = false;
        return m_serial.isOpen() and sendRawCommand({ 0xFF, 0x04, 0x00 });
    }

    bool ControllerGroundSlider::sendLED() {
        if (not m_serial.isOpen()) {
            return false;
        }

        Array<uint8> packet;
        packet.reserve(4 + 1 + 93);
        packet << 0xFF << 0x02 << static_cast<uint8>(1 + 31 * 3) << m_brightness;
        for (const auto& c : m_color) {
            static const auto clamp = [](uint8 v) -> uint8 {
                return std::clamp(v, uint8{ 0 }, uint8{ 0xFC });
            };
            packet << clamp(c.b) << clamp(c.r) << clamp(c.g);
        }
        sendRawCommand(packet);
        return true;
    }

    bool ControllerGroundSlider::sendRawCommand(const Array<uint8>& body) {
        const auto checksum = Checksum(std::span<const uint8>(body.data(), body.size()));
        if (m_serial.write(body.data(), body.size()) != body.size()) {
            return false;
        }
        return m_serial.writeByte(checksum);
    }

    void ControllerGroundSlider::pumpRx() {
        if (m_serial.available()) {
            m_buffer.append(m_serial.readBytes());
        }
    }

    bool ControllerGroundSlider::consumeOnePacket() {
        if (not m_sync) {
            // Seek sync byte (0xFF)
            const auto sync = std::find(m_buffer.begin(), m_buffer.end(), 0xFF);
            if (sync == m_buffer.end()) {
                return false;
            }
            if (sync != m_buffer.begin()) {
                m_buffer.erase(m_buffer.begin(), sync);
            }

            m_sync = true;
        }

        if (m_buffer.size() < 3) {
            return false;
        }
        const uint8 cmd = m_buffer[1];
        const uint8 len = m_buffer[2];
        const size_t need = static_cast<size_t>(3 + len + 1); // consume checksum byte (no check)

        if (m_buffer.size() < need) {
            return false;
        }

        m_last_packet.cmd = cmd;
        m_last_packet.len = len;
        m_last_packet.payload = Array(m_buffer.begin() + 3, m_buffer.begin() + 3 + len);
        m_buffer.erase(m_buffer.begin(), m_buffer.begin() + need);
        m_sync = false;

        return true;
    }

    bool ControllerGroundSlider::consumeUntilOnePacket(uint32 timeout_ms) {
        m_buffer.clear();
        const auto start = Time::GetMillisec();
        bool ok = false;
        while (Time::GetMillisec() - start < timeout_ms) {
            pumpRx();
            if (consumeOnePacket()) {
                ok = true;
                break;
            }
            System::Sleep(1);
        }
        return ok;
    }
}
