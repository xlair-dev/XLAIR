#include <ApiClient/ClientOptions.hpp>

#include <Siv3D/Char.hpp>
#include <cmath>

namespace xlair::api {
    s3d::Optional<OptionsError> ValidateClientOptions(const ClientOptions& options) {
        const auto& endpoint = options.endpoint;
        const std::size_t scheme_size = endpoint.starts_with(U"https://")  ? 8
                                        : endpoint.starts_with(U"http://") ? 7
                                                                           : 0;
        if (scheme_size == 0 || endpoint.size() <= scheme_size || endpoint[scheme_size] == U'/' ||
            endpoint.contains(U'?') || endpoint.contains(U'#') || endpoint.contains(U'@') || endpoint.contains(U'\\') ||
            endpoint.any(s3d::IsSpace)) {
            return OptionsError{ U"endpoint", U"Must be an HTTP(S) base URL without credentials, query or fragment." };
        }
        if (!std::isfinite(options.timeout_seconds) || options.timeout_seconds <= 0.0) {
            return OptionsError{ U"timeout_seconds", U"Must be positive and finite." };
        }

        const auto& auth = options.auth;
        if (!auth.domain.isEmpty() || !auth.client_id.isEmpty() || !auth.client_secret.isEmpty()) {
            if (auth.domain.isEmpty() || auth.client_id.isEmpty() || auth.client_secret.isEmpty() ||
                auth.audience.isEmpty()) {
                return OptionsError{ U"auth", U"Requires domain, client_id, client_secret and audience." };
            }
            if (auth.domain.contains(U'/') || auth.domain.contains(U'@') || auth.domain.contains(U'?') ||
                auth.domain.contains(U'#') || auth.domain.contains(U'\\') || auth.domain.contains(U':') ||
                auth.domain.any(s3d::IsSpace)) {
                return OptionsError{ U"auth.domain", U"The authentication domain must be a hostname." };
            }
        }
        return s3d::none;
    }
}
