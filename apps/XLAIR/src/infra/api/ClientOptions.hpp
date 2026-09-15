#pragma once

#include "app/config/Config.hpp"
#include <ApiClient/ClientOptions.hpp>

namespace xlair::infra::api {
    [[nodiscard]]
    inline xlair::api::ClientOptions ToClientOptions(const app::Config::Api& config) {
        return {
            .endpoint = config.endpoint,
            .timeout_seconds = config.timeout_seconds,
            .auth = {
                .domain = config.auth.domain,
                .client_id = config.auth.client_id,
                .client_secret = config.auth.client_secret,
                .audience = config.auth.audience,
            },
        };
    }
}
