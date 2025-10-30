#pragma once
#include "app/interfaces/IController.hpp"

namespace infra::controller {
    class ControllerGroundSlider : public app::interfaces::IController {
    private:
        struct Packet {
            uint8 cmd = 0;
            uint8 len = 0;
            Array<uint8> payload;
        };

        struct TouchFrame {
            std::array<uint8, 32> zones{};
            uint64 timestamp_ms = 0;
        };

        struct HWInfo {
            String model;
            uint8 device_class;
            String chip_part_number;
            uint8 unk_0xe;
            uint8 firmware_verison;
            uint8 unk_0x10;
            uint8 unk_0x11;
            bool valid = false;
        };

    public:
        ControllerGroundSlider(StringView port, int32 baudrate);

        ~ControllerGroundSlider();

        bool open() override;

        void close() override;

        bool initialize(uint32 timeout_ms = 1000) override;

        void update() override;

        bool setLED(const Array<Color>& color, uint8 brightness) override;

        uint32 sliderPressedFrameCount(size_t index) override;

        uint32 sidePressedFrameCount(SideButton button) override;

    private:
        bool startInput();

        bool stopInput();

        bool sendRawCommand(const Array<uint8>& body);

        void pumpRx();

        bool consumeOnePacket();

        bool consumeUntilOnePacket(uint32 timeout_ms);

        Serial m_serial;
        String m_port;
        int32 m_baudrate;

        bool m_initialized = false;
        HWInfo m_hw{};
        Array<uint8> m_buffer;
        Packet m_last_packet{};
        std::deque<TouchFrame> m_queue;
        uint8 m_brightness = 0x3F;
        bool m_sync = false;
        bool m_enable_input = false;

        static constexpr inline int32 MaxQueue = 128;

        std::array<uint32, 32> m_slider_pressed_frames{};
    };
}
