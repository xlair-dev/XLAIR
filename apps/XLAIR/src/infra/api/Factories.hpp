#pragma once

#include "app/config/Config.hpp"

#include <ApiClient/IClient.hpp>

#include <memory>

namespace xlair::infra::api {
    [[nodiscard]]
    std::unique_ptr<xlair::api::IClient> CreateClient(const app::Config::Api& config);
}
