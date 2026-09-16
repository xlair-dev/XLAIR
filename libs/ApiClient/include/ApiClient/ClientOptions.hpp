#pragma once

#include <Siv3D/String.hpp>
#include <Siv3D/URL.hpp>
#include <Siv3D/Optional.hpp>

namespace xlair::api {
    struct AuthOptions {
        s3d::String domain;
        s3d::String client_id;
        s3d::String client_secret;
        s3d::String audience = U"https://api.xlair.dev";
    };

    struct ClientOptions {
        s3d::URL endpoint = U"https://api.xlair.dev";
        double timeout_seconds = 10.0;
        AuthOptions auth;
    };

    struct OptionsError {
        // Relative to ClientOptions, e.g. "endpoint" or "auth.domain".
        s3d::String field;
        s3d::String message;
    };

    // Empty credentials are allowed until authentication is requested; partial credentials are invalid.
    [[nodiscard]]
    s3d::Optional<OptionsError> ValidateClientOptions(const ClientOptions& options);
}
