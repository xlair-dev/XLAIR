#pragma once

#include <ApiClient/Types.hpp>

namespace xlair::api {
    // Throws s3d::Error for an invalid response. The HTTP boundary converts it to ApiError.
    bool ParseHealth(const s3d::JSON& json);
    User ParseUser(const s3d::JSON& json);
    s3d::Array<UserRecord> ParseRecords(const s3d::JSON& json);
    PlayOptions ParsePlayOptions(const s3d::JSON& json);
    s3d::Array<Music> ParseCatalog(const s3d::JSON& json);
    s3d::uint32 ParseCredits(const s3d::JSON& json);
    s3d::JSON SerializeUserRegistration(s3d::StringView card_id, s3d::StringView display_name, bool is_public);
    s3d::JSON SerializeRecords(s3d::StringView user_id, const s3d::Array<RecordSubmission>& records);
    s3d::JSON SerializePlayOptions(const PlayOptions& options);
}
