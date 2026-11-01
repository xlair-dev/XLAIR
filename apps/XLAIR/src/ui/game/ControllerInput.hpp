#pragma once

#include "app/controller/Controller.hpp"
#include "core/judgement/JudgeEngine.hpp"

#include <array>
#include <cstddef>

namespace xlair::ui::game {
    struct ControllerInput {
        struct Pressed {
            uint32 slider_pressed = 0;
            uint8 side_pressed = 0;
        };

        std::array<core::judgement::TouchEvent, app::controller::TouchZoneCount * 2> touches;
        std::array<core::judgement::SideEvent, app::controller::SideButtonCount> side_down;
        std::size_t touch_count = 0;
        std::size_t side_count = 0;
        Pressed pressed;

        void read(const app::controller::Controller* controller, int64 judged_sample);
    };
}
