#pragma once
#include "core/features/ApiCall.hpp"
#include "core/types/UserData.hpp"

namespace infra::api {
    using core::features::ParseResult;
    using core::types::UserData;

    ParseResult<UserData> ParseUserData(const JSON& json);
}
