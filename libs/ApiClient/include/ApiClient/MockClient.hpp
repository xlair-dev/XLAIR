#pragma once

#include <ApiClient/IClient.hpp>

namespace xlair::api {
    struct MockClientOptions {
        s3d::FilePath data_directory;
    };

    // In-process development backend. JSON provides initial state; mutations live in memory.
    class MockClient final : public IClient {
    public:
        explicit MockClient(const MockClientOptions& options);

        Request<User> findUserByCard(s3d::StringView card_id) override;
        Request<s3d::Array<UserRecord>> fetchUserRecords(s3d::StringView user_id) override;
        Request<s3d::Array<UserRecord>>
        submitPlayRecords(s3d::StringView user_id, const s3d::Array<RecordSubmission>& records) override;
        Request<PlayOptions> fetchPlayOptions(s3d::StringView user_id) override;
        Request<PlayOptions> savePlayOptions(s3d::StringView user_id, const PlayOptions& options) override;
        Request<s3d::uint32> incrementCredits(s3d::StringView user_id) override;
        Request<s3d::Array<Music>> fetchCatalog() override;
        Request<s3d::FilePath> downloadAsset(s3d::URLView url, s3d::FilePathView destination) override;

    private:
        [[nodiscard]]
        bool hasUser(s3d::StringView user_id) const;

        s3d::FilePath m_data_directory;
        s3d::Optional<ApiError> m_load_error;
        s3d::Array<User> m_users;
        s3d::HashTable<s3d::String, s3d::Array<UserRecord>> m_records;
        s3d::HashTable<s3d::String, PlayOptions> m_play_options;
        s3d::Array<Music> m_catalog;
    };
}
