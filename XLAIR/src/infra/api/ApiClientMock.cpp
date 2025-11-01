#include "ApiClientMock.hpp"
#include "Parser.hpp"

namespace infra::api {

    ApiCall<core::types::UserData> ApiClientMock::getUserByCard(StringView card) {
        const auto path = FileSystem::PathAppend(BaseDir, U"users/by_card/{}.json"_fmt(card));
        return ApiCall<core::types::UserData>::Mock(path, ParseUserData);
    }


    ApiCall<Array<core::types::Record>> ApiClientMock::getUserRecords(StringView user_id) {
        const auto path = FileSystem::PathAppend(BaseDir, U"users/{}/records.json"_fmt(user_id));
        return ApiCall<Array<core::types::Record>>::Mock(path, ParseUserRecords);
    }

}
