#pragma once
#include "core/types/ApiCall.hpp"

namespace app::interfaces {
    class IApiClient {
    public:

        struct UserData {};

        virtual ~IApiClient() = default;
    };
}
