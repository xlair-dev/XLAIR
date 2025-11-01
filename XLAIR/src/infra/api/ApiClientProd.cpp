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

    ApiCall<bool> ApiClientProd::postUserRecords(StringView user_id, const Array<core::types::Record>& record) {
        const auto url = m_endpoint + U"/users/{}/records"_fmt(user_id);
        JSON json;
        for (const auto& rec : record) {
            JSON record_json;
            record_json[U"userId"] = user_id;
            record_json[U"sheetId"] = rec.sheet_id;
            record_json[U"score"] = rec.score;
            switch (rec.clear_type) {
                case core::types::ClearType::failed:
                    record_json[U"clearType"] = U"failed";
                    break;
                case core::types::ClearType::clear:
                    record_json[U"clearType"] = U"clear";
                    break;
                case core::types::ClearType::fullcombo:
                    record_json[U"clearType"] = U"fullcombo";
                    break;
                case core::types::ClearType::perfect:
                    record_json[U"clearType"] = U"perfect";
                    break;
            }
            json.push_back(record_json);
        }
        const std::string data = json.formatUTF8();
        // TODO: check body
        return ApiCall<bool>::HTTPPost(url, {}, data.data(), data.size(), [](const JSON&) { return ParseResult{ true, true }; });
    }

}
