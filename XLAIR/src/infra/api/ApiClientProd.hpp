﻿#pragma once
#include "app/interfaces/IApiClient.hpp"

using core::features::ApiCall;

namespace infra::api {
    class ApiClientProd : public app::interfaces::IApiClient {
    public:

        ApiClientProd(URLView endpoint);

        ApiCall<core::types::UserData> getUserByCard(StringView card) override;

        ApiCall<Array<core::types::Record>> getUserRecords(StringView user_id) override;

    private:
        URL m_endpoint;
    };
}
