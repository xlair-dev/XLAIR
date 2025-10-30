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
}
