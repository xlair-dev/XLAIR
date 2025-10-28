#pragma once

namespace app::interfaces {
    class IController {
    public:
        enum class SideButton {
            left_up,
            left_bottom,
            right_up,
            right_bottom,
        };

        virtual ~IController() = default;

        virtual bool open(StringView port, int32 baudrate) = 0;

        virtual void close() = 0;

        virtual bool initialize(uint32 timeout_ms) = 0;

        virtual void update() = 0;

        virtual bool setLED(const Array<Color>& color, uint8 brightness) = 0;

        // +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
        // |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |
        // |  0 |  2 |  4 |  6 |  8 | 10 | 12 | 14 | 16 | 18 | 20 | 22 | 24 | 26 | 28 | 30 |
        // |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |
        // +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
        // |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |
        // |  1 |  3 |  5 |  7 |  9 | 11 | 13 | 15 | 17 | 19 | 21 | 23 | 25 | 27 | 29 | 31 |
        // |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |    |
        // +----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+----+
        virtual uint32 sliderPressedFrameCount(size_t index) = 0;

        virtual uint32 sidePressedFrameCount(SideButton button) = 0;
    };
}
