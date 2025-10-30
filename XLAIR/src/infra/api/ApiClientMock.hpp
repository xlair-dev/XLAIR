#pragma once
#include "app/interfaces/IApiClient.hpp"

using core::features::ApiCall;

namespace infra::api {
    class ApiClientMock : public app::interfaces::IApiClient {
    public:

        ApiCall<core::types::UserData> getUserByCard(StringView card) override;

    private:
        static constexpr inline FilePathView BaseDir{ U"mockdata/api/" };
    };
}
