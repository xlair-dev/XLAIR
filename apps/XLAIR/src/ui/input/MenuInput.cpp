#include "MenuInput.hpp"

#include <cmath>
#include <limits>

namespace xlair::ui::input {
    namespace {
        [[nodiscard]]
        uint32 TouchMask(const uint32 start, const uint32 width) {
            constexpr uint32 ColumnCount = app::controller::TouchZoneCount / 2;
            if (start >= ColumnCount || width == 0) {
                return 0;
            }
            const uint32 zone_count = Min(width, ColumnCount - start) * 2;
            return (std::numeric_limits<uint32>::max() >> (32 - zone_count)) << (start * 2);
        }
    }

    void MenuInput::update(
        const app::controller::Controller* controller,
        const std::span<const Input> keys,
        const double delta_seconds,
        const bool enabled,
        const bool include_side_buttons
    ) {
        m_touch_down = 0;
        m_touch_pressed = 0;
        m_side_down = 0;
        m_accept_input = false;
        bool pressed = false;
        bool available = true;
        if (controller) {
            for (std::size_t zone = 0; zone < app::controller::TouchZoneCount; ++zone) {
                const auto& state = controller->touchZone(zone);
                pressed |= state.pressed();
                available &= state.available();
                if (state.down()) {
                    m_touch_down |= uint32{ 1 } << zone;
                }
                if (state.pressed()) {
                    m_touch_pressed |= uint32{ 1 } << zone;
                }
            }
            for (std::size_t index = 0; include_side_buttons && index < app::controller::SideButtonCount; ++index) {
                const auto& state = controller->sideButton(static_cast<app::controller::SideButton>(index));
                pressed |= state.pressed();
                if (state.down()) {
                    m_side_down |= uint32{ 1 } << index;
                }
            }
        }
        for (const auto& key : keys) {
            pressed |= key.pressed();
        }

        // Disabled screens and missing measurements must not rearm the next gesture.
        if (!enabled || !available) {
            m_armed = false;
            m_release_seconds = 0.0;
            m_touch_pressed = 0;
        } else if (pressed) {
            m_accept_input = m_armed;
            m_armed = false;
            m_release_seconds = 0.0;
        } else if (std::isfinite(delta_seconds) && delta_seconds > 0.0) {
            m_release_seconds = Min(ReleaseSeconds, m_release_seconds + delta_seconds);
            m_armed = m_release_seconds >= ReleaseSeconds;
        }
    }

    bool MenuInput::keyDown(const Input& key) {
        if (!m_accept_input || !key.down()) {
            return false;
        }
        m_accept_input = false;
        return true;
    }

    bool MenuInput::touchDown(const uint32 start, const uint32 width) {
        if (!m_accept_input || (m_touch_down & TouchMask(start, width)) == 0) {
            return false;
        }
        m_accept_input = false;
        return true;
    }

    bool MenuInput::sideDown(const app::controller::SideButton button) {
        const auto index = static_cast<std::size_t>(button);
        if (!m_accept_input || index >= app::controller::SideButtonCount ||
            (m_side_down & (uint32{ 1 } << index)) == 0) {
            return false;
        }
        m_accept_input = false;
        return true;
    }

    bool MenuInput::touchPressed(const uint32 start, const uint32 width) const {
        return (m_touch_pressed & TouchMask(start, width)) != 0;
    }

}
