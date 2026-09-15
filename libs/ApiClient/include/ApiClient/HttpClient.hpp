#pragma once

#include <ApiClient/ClientOptions.hpp>
#include <ApiClient/IClient.hpp>

namespace xlair::api {
    struct HttpSession;

    class HttpClient final : public IClient {
    public:
        explicit HttpClient(const ClientOptions& config);

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
        std::shared_ptr<HttpSession> m_session;
    };
}
