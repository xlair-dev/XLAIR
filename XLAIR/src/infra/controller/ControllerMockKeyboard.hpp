#pragma once
#include "app/interfaces/IController.hpp"

namespace infra {
    class ControllerMockKeyboard : public app::interfaces::IController {
    public:
        bool open(StringView port, int32 baudrate) override;

        void close() override;

        bool initialize(uint32 timeout_ms = 1000) override;

        void update() override;

        bool setLED(const Array<Color>& color, uint8 brightness) override;

        uint32 sliderPressedFrameCount(size_t index) override;

        uint32 sidePressedFrameCount(SideButton button) override;

    private:
        std::array<uint32, 32> m_slider_pressed_frames{};
        std::array<uint32, 4> m_side_pressed_frames{};
    };
}
