#pragma once

#include <Siv3D.hpp>

#include <memory>
#include <variant>

namespace xlair::api {
    enum class ErrorKind {
        Configuration,
        Authentication,
        Network,
        Timeout,
        Http,
        InvalidResponse,
        Cancelled,
    };

    struct ApiError {
        ErrorKind kind;
        s3d::String message;
        s3d::Optional<s3d::int32> status_code;
    };

    struct RequestProgress {
        s3d::int64 downloaded_bytes = 0;
        s3d::Optional<s3d::int64> download_total_bytes;
        std::size_t completed_files = 0;
        std::size_t total_files = 0;
    };

    template <class T> using Result = std::variant<T, ApiError>;

    // Poll from the application update loop. A terminal result remains available until destruction.
    // Requests are owned by the caller; all methods are called on the main thread.
    template <class T> class IRequest {
    public:
        virtual ~IRequest() = default;
        virtual void update() = 0;
        virtual void cancel() = 0;
        [[nodiscard]]
        virtual const s3d::Optional<Result<T>>& result() const noexcept = 0;
        [[nodiscard]]
        virtual s3d::Optional<RequestProgress> progress() const noexcept {
            return s3d::none;
        }
    };

    template <class T> using Request = std::unique_ptr<IRequest<T>>;
}
