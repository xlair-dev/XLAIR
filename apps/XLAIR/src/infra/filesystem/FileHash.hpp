#pragma once

#include <picosha2.h>

#include <cstdint>
#include <span>
#include <string>

namespace xlair::infra::filesystem {
    class SHA256Hasher {
    public:
        void update(const std::span<const std::uint8_t> bytes) {
            m_hasher.process(bytes.begin(), bytes.end());
        }

        [[nodiscard]]
        std::string finish() {
            m_hasher.finish();
            return picosha2::get_hash_hex_string(m_hasher);
        }

    private:
        picosha2::hash256_one_by_one m_hasher;
    };
}
