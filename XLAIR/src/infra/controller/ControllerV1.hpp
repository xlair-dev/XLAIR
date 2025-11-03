#pragma once
#include "app/interfaces/IController.hpp"

namespace infra::controller {
    class ControllerV1 : public app::interfaces::IController {
    public:
        ControllerV1(StringView port, int32 baudrate);

        ~ControllerV1();

        bool open() override;

        void close() override;

        bool initialize(uint32 timeout_ms) override;

        void update() override;

        bool setLED(const Array<Color>& color, uint8 brightness) override;

        uint32 sliderPressedFrameCount(size_t index) override;

        uint32 sidePressedFrameCount(SideButton button) override;

        uint64 lastInputTimestampMs() const override;

    private:
        Serial m_serial;
        String m_port;
        int32 m_baudrate;

        bool m_initialized = false;
        int32 m_t = 0;

        std::array<uint32, 32> m_slider_pressed_frames{};
        std::array<bool, 32> m_slider_state{};
    };
}
