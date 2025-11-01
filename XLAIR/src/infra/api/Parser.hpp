#pragma once
#include "core/features/ApiCall.hpp"
#include "core/types/UserData.hpp"
#include "core/types/Record.hpp"

namespace infra::api {
    using core::features::ParseResult;
    using core::types::UserData;
    using Records = Array<core::types::Record>;

    ParseResult<UserData> ParseUserData(const JSON& json);

    ParseResult<Records> ParseUserRecords(const JSON& json);
}
