#include "ControllerMockKeyboard.hpp"

namespace infra {
    bool ControllerMockKeyboard::open(StringView, int32) {
        return true; // do nothing
    }

    void ControllerMockKeyboard::close() {
        // do nothing
    }

    bool ControllerMockKeyboard::initialize(uint32) {
        return true; // do nothing
    }

    void ControllerMockKeyboard::update() {
        constexpr std::array<Input, 32> Keys{
            Key1, KeyA,
            KeyQ, KeyZ,
            Key2, KeyS,
            KeyW, KeyX,

            Key3, KeyD,
            KeyE, KeyC,
            Key4, KeyF,
            KeyR, KeyV,

            Key5, KeyG,
            KeyT, KeyB,
            Key6, KeyH,
            KeyY, KeyN,

            Key7, KeyJ,
            KeyU, KeyM,
            Key8, KeyK,
            KeyI, KeyComma,
        };
        
        for (size_t i = 0; i < 32; ++i) {
            if (Keys[i].pressed()) {
                m_slider_pressed_frames[i]++;
            } else {
                m_slider_pressed_frames[i] = 0;
            }
        }

        constexpr std::array<Input, 4> SideKeys{
            Key9, // left_up
            KeyO, // left_bottom
            Key0, // right_up
            KeyP, // right_bottom
        };

        for (size_t i = 0; i < 4; ++i) {
            if (SideKeys[i].pressed()) {
                m_side_pressed_frames[i]++;
            } else {
                m_side_pressed_frames[i] = 0;
            }
        }
    }

    bool ControllerMockKeyboard::setLED(const Array<Color>&, uint8) {
        return true; // do nothing
    }

    uint32 ControllerMockKeyboard::sliderPressedFrameCount(size_t index) {
        return m_slider_pressed_frames[index];
    }

    uint32 ControllerMockKeyboard::sidePressedFrameCount(SideButton button) {
        return m_side_pressed_frames[static_cast<size_t>(button)];
    }

}
