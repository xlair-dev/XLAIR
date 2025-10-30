#pragma once
#include "core/features/ApiCall.hpp"
#include "core/types/UserData.hpp"

namespace app::interfaces {
    class IApiClient {
    public:

        virtual ~IApiClient() = default;

        virtual core::features::ApiCall<core::types::UserData> getUserByCard(StringView card) = 0;
    };
}
