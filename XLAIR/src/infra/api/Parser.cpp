#include "Parser.hpp"

namespace infra::api {
    namespace {
        template<typename T>
        inline bool helper(const JSON& json, StringView key, T& entry) {
            if (json.hasElement(key)) {
                entry = json[key].get<T>();
                return true;
            }
            return false;
        }
    }

    ParseResult<UserData> ParseUserData(const JSON& json) {
        UserData u;
        bool ok = true;
        ok &= helper(json, U"id", u.id);
        ok &= helper(json, U"card", u.card);
        ok &= helper(json, U"displayName", u.displayname);
        ok &= helper(json, U"rating", u.rating);
        ok &= helper(json, U"xp", u.xp);
        ok &= helper(json, U"credits", u.credits);
        ok &= helper(json, U"isPublic", u.isPublic);
        ok &= helper(json, U"isAdmin", u.isAdmin);
        return { u, ok };
    }

    ParseResult<Records> ParseUserRecords(const JSON& json) {
        Records r;
        bool ok = json.isArray();
        if (ok) {
            for (const auto& [index, object] : json) {
                core::types::Record record;
                bool record_ok = true;
                record_ok &= helper(object, U"id", record.id);
                record_ok &= helper(object, U"sheetId", record.sheet_id);
                record_ok &= helper(object, U"score", record.score);

                String clear_type;
                record_ok &= helper(object, U"clearType", clear_type);
                if (clear_type == U"failed") {
                    record.clear_type = core::types::ClearType::failed;
                } else if (clear_type == U"clear") {
                    record.clear_type = core::types::ClearType::clear;
                } else if (clear_type == U"fullcombo") {
                    record.clear_type = core::types::ClearType::fullcombo;
                } else if (clear_type == U"perfect") {
                    record.clear_type = core::types::ClearType::perfect;
                } else {
                    record_ok = false;
                }
                record_ok &= helper(object, U"playCount", record.play_count);
                if (record_ok) {
                    r.push_back(record);
                }
                ok &= record_ok;
            }
        }
        return { r, ok };
    }
}
