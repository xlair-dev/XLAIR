#pragma once

#include "app/controller/Controller.hpp"

#include <span>

namespace xlair::ui::input {
    // One UI action per gesture, shared by slider regions, side buttons, and shortcut keys.
    class MenuInput {
    public:
        void update(
            const app::controller::Controller* controller,
            std::span<const Input> keys,
            double delta_seconds,
            bool enabled = true,
            bool include_side_buttons = false
        );

        [[nodiscard]]
        bool keyDown(const Input& key);

        [[nodiscard]]
        bool touchDown(uint32 start, uint32 width);

        [[nodiscard]]
        bool sideDown(app::controller::SideButton button);

        [[nodiscard]]
        bool touchPressed(uint32 start, uint32 width) const;

    private:
        static constexpr double ReleaseSeconds = 0.05;

        uint32 m_touch_down = 0;
        uint32 m_touch_pressed = 0;
        uint32 m_side_down = 0;
        double m_release_seconds = 0.0;
        bool m_armed = false;
        bool m_accept_input = false;
    };
}
