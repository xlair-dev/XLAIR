#pragma once

#include <Siv3D.hpp>
#include <cstddef>
#include <utility>

namespace xlair::sheets {
    enum class DiagnosticSeverity : s3d::uint8 {
        Warning,
        Error,
    };

    struct Diagnostic {
        DiagnosticSeverity severity = DiagnosticSeverity::Error;
        s3d::String message;
        s3d::FilePath path;
        s3d::Optional<std::size_t> line;
        s3d::Optional<std::size_t> column;
    };

    template <class T> struct Result {
        s3d::Optional<T> value;
        s3d::Array<Diagnostic> diagnostics;

        Result() = default;

        explicit Result(T success_value) : value{ std::move(success_value) } {}

        [[nodiscard]] static Result makeError(s3d::String message, s3d::FilePath path = U"",
                                              s3d::Optional<std::size_t> line = s3d::none,
                                              s3d::Optional<std::size_t> column = s3d::none) {
            Result result;
            result.diagnostics.push_back({
                .severity = DiagnosticSeverity::Error,
                .message = std::move(message),
                .path = std::move(path),
                .line = line,
                .column = column,
            });
            return result;
        }

        [[nodiscard]] bool hasValue() const noexcept {
            return static_cast<bool>(value);
        }

        [[nodiscard]] explicit operator bool() const noexcept {
            return hasValue();
        }

        [[nodiscard]] T& operator*() & {
            return *value;
        }

        [[nodiscard]] const T& operator*() const& {
            return *value;
        }

        [[nodiscard]] T* operator->() {
            return &*value;
        }

        [[nodiscard]] const T* operator->() const {
            return &*value;
        }
    };
}
