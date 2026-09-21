#pragma once

#include "Common.hpp"

#include <limits>

namespace xlair::app::credits {
    class CreditPool {
    public:
        void insert() noexcept {
            if (m_pending < std::numeric_limits<uint32>::max()) {
                ++m_pending;
            }
        }

        [[nodiscard]]
        bool available() const noexcept {
            return m_pending > 0;
        }

        void consume() noexcept {
            if (!available()) {
                return;
            }

            --m_pending;
        }

        [[nodiscard]]
        uint32 pending() const noexcept {
            return m_pending;
        }

    private:
        uint32 m_pending = 0;
    };
}
