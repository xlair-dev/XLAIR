#include <ApiClient/MockClient.hpp>

#include "Parser.hpp"

#include <algorithm>
#include <cmath>

namespace xlair::api {
    namespace {
        constexpr auto MockRecordTimestamp = U"2026-09-16T00:00:00Z";

        template <class T> class CompletedRequest final : public IRequest<T> {
        public:
            explicit CompletedRequest(Result<T> result) : m_result{ std::move(result) } {}
            void update() override {}
            void cancel() override {}
            const s3d::Optional<Result<T>>& result() const noexcept override {
                return m_result;
            }

        private:
            s3d::Optional<Result<T>> m_result;
        };

        template <class T> Request<T> Complete(Result<T> result) {
            return std::make_unique<CompletedRequest<T>>(std::move(result));
        }

        ApiError Error(ErrorKind kind, s3d::String message, s3d::Optional<s3d::int32> status = s3d::none) {
            return { kind, std::move(message), status };
        }

        s3d::JSON LoadRequired(const s3d::FilePath& path) {
            const auto json = s3d::JSON::Load(path);
            if (!json) {
                throw s3d::Error{ U"Failed to load mock data: " + path };
            }
            return json;
        }

        bool IsSafeAssetURL(s3d::URLView url) {
            if ((!url.starts_with(U"/musics/") && !url.starts_with(U"/sheets/")) || url.contains(U'\\')) {
                return false;
            }
            return s3d::String{ url.substr(1) }.split(U'/').all([](const s3d::String& part) {
                return !part.isEmpty() && part != U"." && part != U".." && part.all([](char32_t ch) {
                    return s3d::IsAlnum(ch) || ch == U'-' || ch == U'_' || ch == U'.';
                });
            });
        }

        int ClearRank(ClearType type) {
            return static_cast<int>(type);
        }
    }

    MockClient::MockClient(const MockClientOptions& options)
        : m_data_directory{ s3d::FileSystem::FullPath(options.data_directory) } {
        try {
            m_catalog = ParseCatalog(LoadRequired(m_data_directory + U"sync.json"));
            const auto state = LoadRequired(m_data_directory + U"state.json");
            if (!state.isObject() || !state[U"users"].isArray() || !state[U"records"].isArray() ||
                !state[U"playOptions"].isArray()) {
                throw s3d::Error{ U"Invalid mock state root." };
            }
            for (const auto& entry : state[U"users"]) {
                m_users.push_back(ParseUser(entry.value));
            }
            for (const auto& entry : state[U"records"]) {
                const auto user_id = entry.value[U"userId"].getOpt<s3d::String>();
                if (!user_id) {
                    throw s3d::Error{ U"Mock record requires userId." };
                }
                s3d::JSON array = s3d::JSON::Parse(U"[]");
                array.push_back(entry.value);
                m_records[*user_id].push_back(ParseRecords(array).front());
            }
            for (const auto& entry : state[U"playOptions"]) {
                const auto user_id = entry.value[U"userId"].getOpt<s3d::String>();
                if (!user_id) {
                    throw s3d::Error{ U"Mock play options require userId." };
                }
                m_play_options[*user_id] = ParsePlayOptions(entry.value);
            }
        } catch (const s3d::Error&) {
            m_load_error = Error(ErrorKind::InvalidResponse, U"Failed to load mock API data.");
        }
    }

    Request<User> MockClient::findUserByCard(s3d::StringView card_id) {
        if (m_load_error) {
            return Complete<User>(*m_load_error);
        }
        const auto found = std::find_if(m_users.begin(), m_users.end(), [&](const User& user) {
            return user.card == card_id;
        });
        return found == m_users.end() ? Complete<User>(Error(ErrorKind::Http, U"Mock user not found.", 404))
                                      : Complete<User>(*found);
    }

    bool MockClient::hasUser(s3d::StringView user_id) const {
        return std::any_of(m_users.begin(), m_users.end(), [&](const User& user) {
            return user.id == user_id;
        });
    }

    Request<s3d::Array<UserRecord>> MockClient::fetchUserRecords(s3d::StringView user_id) {
        if (m_load_error) {
            return Complete<s3d::Array<UserRecord>>(*m_load_error);
        }
        if (!hasUser(user_id)) {
            return Complete<s3d::Array<UserRecord>>(Error(ErrorKind::Http, U"Mock user not found.", 404));
        }
        return Complete<s3d::Array<UserRecord>>(m_records[s3d::String{ user_id }]);
    }

    Request<s3d::Array<UserRecord>>
    MockClient::submitPlayRecords(s3d::StringView user_id, const s3d::Array<RecordSubmission>& submissions) {
        if (m_load_error) {
            return Complete<s3d::Array<UserRecord>>(*m_load_error);
        }
        if (submissions.isEmpty()) {
            return Complete<s3d::Array<UserRecord>>(s3d::Array<UserRecord>{});
        }
        if (!hasUser(user_id)) {
            return Complete<s3d::Array<UserRecord>>(Error(ErrorKind::Http, U"Mock user not found.", 404));
        }
        auto& stored = m_records[s3d::String{ user_id }];
        s3d::Array<UserRecord> responses;
        responses.reserve(submissions.size());
        for (const auto& submission : submissions) {
            auto found = std::find_if(stored.begin(), stored.end(), [&](const UserRecord& record) {
                return record.sheet_id == submission.sheet_id;
            });
            if (found == stored.end()) {
                stored.push_back(
                    {
                        .id = U"mock-record-" + submission.sheet_id,
                        .sheet_id = submission.sheet_id,
                        .score = submission.score,
                        .clear_type = submission.clear_type,
                        .play_count = 1,
                        .updated_at = MockRecordTimestamp,
                    }
                );
                responses.push_back(stored.back());
            } else {
                found->score = s3d::Max(found->score, submission.score);
                if (ClearRank(submission.clear_type) > ClearRank(found->clear_type)) {
                    found->clear_type = submission.clear_type;
                }
                ++found->play_count;
                found->updated_at = MockRecordTimestamp;
                responses.push_back(*found);
            }
        }
        return Complete<s3d::Array<UserRecord>>(std::move(responses));
    }

    Request<PlayOptions> MockClient::fetchPlayOptions(s3d::StringView user_id) {
        if (m_load_error) {
            return Complete<PlayOptions>(*m_load_error);
        }
        if (!hasUser(user_id)) {
            return Complete<PlayOptions>(Error(ErrorKind::Http, U"Mock user not found.", 404));
        }
        const auto found = m_play_options.find(s3d::String{ user_id });
        return found == m_play_options.end() ? Complete<PlayOptions>(PlayOptions{})
                                             : Complete<PlayOptions>(found->second);
    }

    Request<PlayOptions> MockClient::savePlayOptions(s3d::StringView user_id, const PlayOptions& options) {
        if (m_load_error) {
            return Complete<PlayOptions>(*m_load_error);
        }
        if (!hasUser(user_id)) {
            return Complete<PlayOptions>(Error(ErrorKind::Http, U"Mock user not found.", 404));
        }
        if (!std::isfinite(options.note_speed)) {
            return Complete<PlayOptions>(Error(ErrorKind::InvalidResponse, U"Mock play options are invalid."));
        }
        m_play_options[s3d::String{ user_id }] = options;
        return Complete<PlayOptions>(options);
    }

    Request<s3d::uint32> MockClient::incrementCredits(s3d::StringView user_id) {
        if (m_load_error) {
            return Complete<s3d::uint32>(*m_load_error);
        }
        const auto found = std::find_if(m_users.begin(), m_users.end(), [&](const User& user) {
            return user.id == user_id;
        });
        if (found == m_users.end()) {
            return Complete<s3d::uint32>(Error(ErrorKind::Http, U"Mock user not found.", 404));
        }
        return Complete<s3d::uint32>(++found->credits);
    }

    Request<s3d::Array<Music>> MockClient::fetchCatalog() {
        return m_load_error ? Complete<s3d::Array<Music>>(*m_load_error) : Complete<s3d::Array<Music>>(m_catalog);
    }

    Request<s3d::FilePath> MockClient::downloadAsset(s3d::URLView url, s3d::FilePathView destination) {
        if (m_load_error) {
            return Complete<s3d::FilePath>(*m_load_error);
        }
        if (!IsSafeAssetURL(url)) {
            return Complete<s3d::FilePath>(Error(ErrorKind::Configuration, U"Invalid mock asset URL."));
        }
        const auto source = m_data_directory + U"assets/" + url.substr(1);
        if (!s3d::FileSystem::IsFile(source)) {
            return Complete<s3d::FilePath>(Error(ErrorKind::Http, U"Mock asset not found.", 404));
        }
        if (!s3d::FileSystem::Copy(source, destination, s3d::CopyOption::OverwriteExisting)) {
            return Complete<s3d::FilePath>(Error(ErrorKind::Network, U"Failed to copy mock asset."));
        }
        return Complete<s3d::FilePath>(s3d::FilePath{ destination });
    }
}
