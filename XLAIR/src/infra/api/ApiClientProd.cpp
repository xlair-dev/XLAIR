#include "ApiClientProd.hpp"
#include "Parser.hpp"

namespace infra::api {

    ApiClientProd::ApiClientProd(URLView endpoint) : m_endpoint(endpoint) {}

    ApiCall<core::types::UserData> ApiClientProd::getUserByCard(StringView card) {
        const auto url = m_endpoint + U"/users?card={}"_fmt(card);
        return ApiCall<core::types::UserData>::HTTPGet(url, {}, ParseUserData);
    }

    ApiCall<Array<core::types::Record>> ApiClientProd::getUserRecords(StringView user_id) {
        const auto url = m_endpoint + U"/users/{}/records"_fmt(user_id);
        return ApiCall<Array<core::types::Record>>::HTTPGet(url, {}, ParseUserRecords);
    }

}
