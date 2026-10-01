#include "Factories.hpp"

#include "ClientOptions.hpp"
#include <ApiClient/HttpClient.hpp>
#include <ApiClient/MockClient.hpp>

namespace xlair::infra::api {
    std::unique_ptr<xlair::api::IClient> CreateClient(const app::Config::Api& config) {
        if (config.mode == app::Config::Api::Mode::Mock) {
            return std::make_unique<xlair::api::MockClient>(xlair::api::MockClientOptions{
                .data_directory = config.mock.data_directory,
            });
        }
        return std::make_unique<xlair::api::HttpClient>(ToClientOptions(config));
    }
}
