#pragma once

#include "Common.hpp"

#include <memory>
#include <variant>

namespace xlair::app::card {
    enum class ErrorKind {
        Unavailable,
        ReaderNotFound,
        Communication,
        UnsupportedCard,
        Cancelled,
    };

    struct Error {
        ErrorKind kind = ErrorKind::Unavailable;
        String message;
    };

    struct Card {
        String card_id;
    };

    using Result = std::variant<Card, Error>;

    class IScan {
    public:
        virtual ~IScan() = default;

        virtual void update() = 0;
        virtual void cancel() = 0;

        [[nodiscard]]
        virtual const Optional<Result>& result() const noexcept = 0;
    };

    using ScanRequest = std::unique_ptr<IScan>;
}
