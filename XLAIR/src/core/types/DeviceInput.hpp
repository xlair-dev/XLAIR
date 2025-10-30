#pragma once
#include "Common.hpp"

namespace core::types {
    struct DeviceInput {
    public:
        constexpr inline DeviceInput(uint32 curr_frame_count) noexcept
            : m_curr_frame_count(curr_frame_count) {}

        constexpr inline bool down() const noexcept {
            return m_curr_frame_count == 1;
        }

        constexpr inline bool pressed() const noexcept {
            return m_curr_frame_count > 0;
        }

        //constexpr inline bool up() const noexcept {
        //    return m_prev_frame_count > 0 and m_curr_frame_count == 0;
        //}

        constexpr inline uint32 frameCount() const noexcept {
            return m_curr_frame_count;
        }

    private:
        //const uint32 m_prev_frame_count;
        const uint32 m_curr_frame_count;
    };
}
