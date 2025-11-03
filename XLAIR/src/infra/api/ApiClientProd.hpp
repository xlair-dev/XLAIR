#pragma once
#include "app/interfaces/IApiClient.hpp"

using core::features::ApiCall;

namespace infra::api {
    class ApiClientProd : public app::interfaces::IApiClient {
    public:

        ApiClientProd(URLView endpoint);

        ApiCall<core::types::UserData> getUserByCard(StringView card) override;

        ApiCall<Array<core::types::Record>> getUserRecords(StringView user_id) override;

        core::features::ApiCall<bool> postUserRecords(StringView user_id, const Array<core::types::Record>& records) override;

    private:
        URL m_endpoint;
    };
}
