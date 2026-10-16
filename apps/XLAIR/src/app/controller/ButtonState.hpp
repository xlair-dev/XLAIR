#pragma once

#include "Types.hpp"

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

        [[nodiscard]]
        bool available() const noexcept {
            return m_available;
        }

    private:
        friend class Controller;

        void update(const bool pressed, const InputStatus status) noexcept {
            if (status == InputStatus::Unavailable) {
                m_previous = false;
                m_current = false;
                m_pressed_frames = 0;
                m_available = false;
                return;
            }

            // The state may have changed while measurements were missing. Resume
            // from the observed state without inventing a down/up edge.
            m_previous = status == InputStatus::Resynchronized ? pressed : m_current;
            m_current = pressed;
            m_available = true;

            if (!m_current) {
                m_pressed_frames = 0;
            } else if (m_pressed_frames < std::numeric_limits<uint32>::max()) {
                ++m_pressed_frames;
            }
        }

        bool m_previous = false;
        bool m_available = true;
        bool m_current = false;
        uint32 m_pressed_frames = 0;
    };
}
