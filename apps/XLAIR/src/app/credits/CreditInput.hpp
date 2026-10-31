#pragma once

#include <cstdint>
#include <optional>

namespace xlair::app::credits {
    class CreditInput {
    public:
        [[nodiscard]]
        bool update(const bool pressed, const std::uint64_t now_ms) noexcept {
            if (pressed) {
                const bool accepted = m_armed;
                m_armed = false;
                m_release_since_ms.reset();
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
        // Rearm only after the whole pulse burst has settled, not a fixed delay after its first edge.
        static constexpr std::uint64_t ReleaseMillisec = 200;

        std::optional<std::uint64_t> m_release_since_ms;
        bool m_armed = true;
    };
}
