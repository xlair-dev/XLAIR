#pragma once

#include "app/card/Types.hpp"

#include <utility>

namespace xlair::infra::card::detail {
    class UnavailableScan final : public app::card::IScan {
    public:
        explicit UnavailableScan(String message)
            : m_result{ app::card::Error{
                  .kind = app::card::ErrorKind::Unavailable,
                  .message = std::move(message),
              } } {}

        void update() override {}

        void cancel() override {}

        [[nodiscard]]
        const Optional<app::card::Result>& result() const noexcept override {
            return m_result;
        }

    private:
        Optional<app::card::Result> m_result;
    };

    [[nodiscard]]
    inline app::card::ScanRequest MakeUnavailableScan(String message) {
        return std::make_unique<UnavailableScan>(std::move(message));
    }
}
