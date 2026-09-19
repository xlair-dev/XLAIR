#pragma once

#include "Common.hpp"

#include <limits>

namespace xlair::app::controller {
    class ButtonState {
    public:
        [[nodiscard]]
        bool down() const noexcept {
            return m_current && !m_previous;
        }

        [[nodiscard]]
        bool pressed() const noexcept {
            return m_current;
        }

        [[nodiscard]]
        bool up() const noexcept {
            return !m_current && m_previous;
        }

        [[nodiscard]]
        uint32 pressedFrames() const noexcept {
            return m_pressed_frames;
        }

    private:
        friend class Controller;

        void update(const bool pressed) noexcept {
            m_previous = m_current;
            m_current = pressed;

            if (!m_current) {
                m_pressed_frames = 0;
            } else if (m_pressed_frames < std::numeric_limits<uint32>::max()) {
                ++m_pressed_frames;
            }
        }

        bool m_previous = false;
        bool m_current = false;
        uint32 m_pressed_frames = 0;
    };
}
