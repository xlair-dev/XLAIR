#pragma once
#define NO_S3D_USING

#include <Siv3D.hpp>
#include <cstddef>

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
