#pragma once

#include <cstdint>
#include <optional>

namespace xlair::app::credits {
    class CreditInput {
    public:
        [[nodiscard]]
        bool update(const bool pressed, const std::uint64_t now_ms) noexcept {
            if (pressed) {
                const bool accepted = m_armed && (!m_last_press_ms || now_ms - *m_last_press_ms >= CooldownMillisec);
                m_armed = false;
                m_release_since_ms.reset();
                if (accepted) {
                    m_last_press_ms = now_ms;
                }
                return accepted;
            }

            if (!m_release_since_ms) {
                m_release_since_ms = now_ms;
            }
            if (now_ms - *m_release_since_ms >= ReleaseMillisec) {
                m_armed = true;
            }
            return false;
        }

        void suspend() noexcept {
            m_armed = false;
            m_release_since_ms.reset();
        }

    private:
        static constexpr std::uint64_t CooldownMillisec = 100;
        static constexpr std::uint64_t ReleaseMillisec = 30;

        std::optional<std::uint64_t> m_last_press_ms;
        std::optional<std::uint64_t> m_release_since_ms;
        bool m_armed = true;
    };
}
