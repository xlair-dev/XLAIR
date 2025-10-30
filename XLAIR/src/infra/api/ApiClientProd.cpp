#include "ApiClientProd.hpp"
#include "Parser.hpp"

namespace infra::api {

    ApiClientProd::ApiClientProd(URLView endpoint) : m_endpoint(endpoint) {}

    ApiCall<core::types::UserData> ApiClientProd::getUserByCard(StringView card) {
        const auto url = m_endpoint + U"/users?card={}"_fmt(card);
        return ApiCall<core::types::UserData>::HTTPGet(url, {}, ParseUserData);
    }

}
