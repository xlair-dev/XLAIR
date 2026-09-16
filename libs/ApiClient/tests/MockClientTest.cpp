#include <ThirdParty/Catch2/catch.hpp>

#include <ApiClient/MockClient.hpp>

namespace api = xlair::api;
using namespace s3d;

namespace {
    struct Directory {
        FilePath path = FileSystem::TemporaryDirectoryPath() + UUIDValue::Generate().str() + U"/";
        ~Directory() {
            FileSystem::Remove(path);
        }

        void create() const {
            REQUIRE(FileSystem::CreateDirectories(path));
            REQUIRE(
                JSON::Parse(UR"([
                {"music":{"id":"music","title":"Mock","artist":"Artist","bpm":120,
                 "genre":"ORIGINAL","jacket":null,"audio":null,
                 "registrationDate":"2026-09-16T00:00:00Z","isTest":true},"sheets":[]}
            ])")
                    .save(path + U"sync.json")
            );
            REQUIRE(
                JSON::Parse(UR"({
                "users":[{"id":"user","card":"card","displayName":"Player","rating":1,"xp":2,
                    "credits":3,"isPublic":false,"isAdmin":false,"createdAt":"now"}],
                "records":[],
                "playOptions":[{"userId":"user","noteSpeed":4.0,"judgmentOffset":-5}]
            })")
                    .save(path + U"state.json")
            );
        }
    };

    template <class T> const api::Result<T>& ResultOf(api::Request<T>& request) {
        request->update();
        REQUIRE(request->result().has_value());
        return *request->result();
    }
}

TEST_CASE("MockClient loads JSON and applies mutations in memory", "[API][Mock]") {
    Directory directory;
    directory.create();
    api::MockClient client{ { directory.path } };

    auto catalog = client.fetchCatalog();
    REQUIRE(std::holds_alternative<Array<api::Music>>(ResultOf(catalog)));
    CHECK(std::get<Array<api::Music>>(ResultOf(catalog))[0].id == U"music");

    auto user_request = client.findUserByCard(U"card");
    REQUIRE(std::holds_alternative<api::User>(ResultOf(user_request)));
    CHECK(std::get<api::User>(ResultOf(user_request)).display_name == U"Player");

    auto missing = client.findUserByCard(U"missing");
    REQUIRE(std::holds_alternative<api::ApiError>(ResultOf(missing)));
    CHECK(std::get<api::ApiError>(ResultOf(missing)).status_code == 404);

    auto credits = client.incrementCredits(U"user");
    REQUIRE(std::holds_alternative<uint32>(ResultOf(credits)));
    CHECK(std::get<uint32>(ResultOf(credits)) == 4);

    auto options = client.fetchPlayOptions(U"user");
    CHECK(std::get<api::PlayOptions>(ResultOf(options)).judgment_offset_ms == -5);
    auto saved = client.savePlayOptions(U"user", { .note_speed = 7.5, .judgment_offset_ms = 12 });
    CHECK(std::get<api::PlayOptions>(ResultOf(saved)).note_speed == 7.5);
    options = client.fetchPlayOptions(U"user");
    CHECK(std::get<api::PlayOptions>(ResultOf(options)).judgment_offset_ms == 12);

    Array<api::RecordSubmission> submissions{ { U"sheet", 900000, api::ClearType::Clear } };
    auto records = client.submitPlayRecords(U"user", submissions);
    REQUIRE(std::get<Array<api::UserRecord>>(ResultOf(records)).size() == 1);
    submissions[0] = { U"sheet", 800000, api::ClearType::FullCombo };
    records = client.submitPlayRecords(U"user", submissions);
    const auto& record = std::get<Array<api::UserRecord>>(ResultOf(records))[0];
    CHECK(record.score == 900000);
    CHECK(record.clear_type == api::ClearType::FullCombo);
    CHECK(record.play_count == 2);
}

TEST_CASE("Mock record submission returns only submitted records in submission order", "[API][Mock]") {
    Directory directory;
    directory.create();
    api::MockClient client{ { directory.path } };
    auto initial = client.submitPlayRecords(U"user", {{U"existing", 100, api::ClearType::Clear}});
    const auto& created = std::get<Array<api::UserRecord>>(ResultOf(initial));
    REQUIRE(created.size() == 1);
    CHECK(created[0].updated_at == U"2026-09-16T00:00:00Z");

    auto request = client.submitPlayRecords(U"user", {
        {U"new", 200, api::ClearType::Clear},
        {U"new", 300, api::ClearType::FullCombo},
    });
    const auto& responses = std::get<Array<api::UserRecord>>(ResultOf(request));
    REQUIRE(responses.size() == 2);
    CHECK(responses[0].sheet_id == U"new");
    CHECK(responses[0].score == 200);
    CHECK(responses[0].play_count == 1);
    CHECK(responses[1].sheet_id == U"new");
    CHECK(responses[1].score == 300);
    CHECK(responses[1].play_count == 2);
    CHECK(responses[1].updated_at == U"2026-09-16T00:00:00Z");

    auto empty = client.submitPlayRecords(U"user", {});
    CHECK(std::get<Array<api::UserRecord>>(ResultOf(empty)).isEmpty());
    auto unknown_empty = client.submitPlayRecords(U"missing", {});
    CHECK(std::get<Array<api::UserRecord>>(ResultOf(unknown_empty)).isEmpty());
    auto stored = client.fetchUserRecords(U"user");
    CHECK(std::get<Array<api::UserRecord>>(ResultOf(stored)).size() == 2);
}

TEST_CASE("MockClient copies only safe local asset URLs", "[API][Mock]") {
    Directory directory;
    directory.create();
    const auto source = directory.path + U"assets/musics/music/audio/hash.wav";
    REQUIRE(FileSystem::CreateDirectories(FileSystem::ParentPath(source)));
    BinaryWriter writer{ source };
    REQUIRE(writer.write("mock", 4) == 4);
    writer.close();
    api::MockClient client{ { directory.path } };
    const auto destination = directory.path + U"download/result.wav";
    REQUIRE(FileSystem::CreateDirectories(FileSystem::ParentPath(destination)));
    auto download = client.downloadAsset(U"/musics/music/audio/hash.wav", destination);
    REQUIRE(std::holds_alternative<FilePath>(ResultOf(download)));
    CHECK(FileSystem::FileSize(destination) == 4);
    download = client.downloadAsset(U"/musics/../state.json", destination);
    CHECK(std::holds_alternative<api::ApiError>(ResultOf(download)));
}

TEST_CASE("MockClient reports invalid fixture data through requests", "[API][Mock]") {
    Directory directory;
    REQUIRE(FileSystem::CreateDirectories(directory.path));
    api::MockClient client{ { directory.path } };
    auto request = client.fetchCatalog();
    REQUIRE(std::holds_alternative<api::ApiError>(ResultOf(request)));
    CHECK(std::get<api::ApiError>(ResultOf(request)).kind == api::ErrorKind::InvalidResponse);
}
