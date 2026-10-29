#include "MenuInput.hpp"

#include <cmath>
#include <limits>

namespace xlair::ui::input {
    void MenuInput::update(
        const app::controller::Controller* controller,
        const std::span<const Input> keys,
        const double delta_seconds,
        const bool enabled
    ) {
        m_touch_down = 0;
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
            }
        }
        for (const auto& key : keys) {
            pressed |= key.pressed();
        }

        // Disabled screens and missing measurements must not rearm the next gesture.
        if (!enabled || !available) {
            m_armed = false;
            m_release_seconds = 0.0;
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
        constexpr uint32 ColumnCount = app::controller::TouchZoneCount / 2;
        if (!m_accept_input || start >= ColumnCount || width == 0) {
            return false;
        }
        const uint32 zone_count = Min(width, ColumnCount - start) * 2;
        const uint32 mask = (std::numeric_limits<uint32>::max() >> (32 - zone_count)) << (start * 2);
        if ((m_touch_down & mask) == 0) {
            return false;
        }
        m_accept_input = false;
        return true;
    }
}
