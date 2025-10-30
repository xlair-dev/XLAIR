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

    ControllerGroundSlider::ControllerGroundSlider(StringView port, int32 baudrate)
        :m_port(port), m_baudrate(baudrate) {}

    ControllerGroundSlider::~ControllerGroundSlider() {
        close();
    }

    bool ControllerGroundSlider::open() {
        close();
        return m_serial.open(m_port, m_baudrate);
    }

    void ControllerGroundSlider::close() {
        if (m_enable_input) {
            stopInput();
        }
        if (m_serial.isOpen()) {
            m_serial.close();
        }
        m_initialized = false;
        m_buffer.clear();
        m_queue.clear();
        m_hw = {};
        m_sync = false;
        m_enable_input = false;
    }

    bool ControllerGroundSlider::initialize(uint32 timeout_ms) {
        if (not m_serial.isOpen()) {
            return false;
        }

        m_initialized = false;
        m_buffer.clear();
        m_queue.clear();
        m_hw = {};
        m_sync = false;
        m_enable_input = false;

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
            m_hw.deviceClass = payload[8];
            m_hw.chipPartNumber = String{ payload.begin() + 9, payload.begin() + 14 };
            m_hw.unk_0xe = payload[14];
            m_hw.firmwareVerison = payload[15];
            m_hw.unk_0x10 = payload[16];
            m_hw.unk_0x11 = payload[17];
            m_hw.valid = true;
        }

        m_initialized = m_hw.valid;
        return m_initialized;
    }

    bool ControllerGroundSlider::sendRawCommand(const Array<uint8>& body) {
        const auto checksum = Checksum(std::span<const uint8>(body.data(), body.size()));
        if (m_serial.write(body.data(), body.size()) != body.size()) {
            return false;
        }
        return m_serial.writeByte(checksum);
    }
}
