#pragma once

#include <ApiClient/Request.hpp>
#include <ApiClient/Types.hpp>

namespace xlair::api {
    class IClient {
    public:
        virtual ~IClient() = default;

        [[nodiscard]]
        virtual Request<User> findUserByCard(s3d::StringView card_id) = 0;

        [[nodiscard]]
        virtual Request<s3d::Array<UserRecord>> fetchUserRecords(s3d::StringView user_id) = 0;

        [[nodiscard]]
        virtual Request<s3d::Array<UserRecord>>
        submitPlayRecords(s3d::StringView user_id, const s3d::Array<RecordSubmission>& records) = 0;

        [[nodiscard]]
        virtual Request<PlayOptions> fetchPlayOptions(s3d::StringView user_id) = 0;

        [[nodiscard]]
        virtual Request<PlayOptions> savePlayOptions(s3d::StringView user_id, const PlayOptions& options) = 0;

        [[nodiscard]]
        virtual Request<s3d::uint32> incrementCredits(s3d::StringView user_id) = 0;

        [[nodiscard]]
        virtual Request<s3d::Array<Music>> fetchCatalog() = 0;
    };
}
