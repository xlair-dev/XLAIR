#pragma once

#include <memory>

#include <ApiClient/HttpClient.hpp>
#include <ApiClient/IClient.hpp>

#include "app/config/Config.hpp"
#include "infra/api/ClientOptions.hpp"

namespace xlair::infra::api {
    [[nodiscard]]
    inline std::unique_ptr<xlair::api::IClient> CreateClient(const app::Config::Api& config) {
        return std::make_unique<xlair::api::HttpClient>(ToClientOptions(config));
    }
}
