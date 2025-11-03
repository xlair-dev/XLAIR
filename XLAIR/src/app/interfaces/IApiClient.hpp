#pragma once
#include "core/features/ApiCall.hpp"
#include "core/types/UserData.hpp"
#include "core/types/Record.hpp"

namespace app::interfaces {
    class IApiClient {
    public:

        virtual ~IApiClient() = default;

        virtual core::features::ApiCall<core::types::UserData> getUserByCard(StringView card) = 0;

        virtual core::features::ApiCall<Array<core::types::Record>> getUserRecords(StringView user_id) = 0;

        virtual core::features::ApiCall<bool> postUserRecords(StringView user_id, const Array<core::types::Record>& records) = 0;
    };
}
