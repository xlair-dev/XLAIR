#include <ThirdParty/Catch2/catch.hpp>

#include "infra/api/LocalCatalogSync.hpp"
#include "infra/filesystem/FileHash.hpp"

namespace api = xlair::api;
using Mode = xlair::app::flows::CatalogSync::Mode;
using namespace s3d;

namespace {
    template <class T> class CompletedRequest final : public api::IRequest<T> {
    public:
        explicit CompletedRequest(api::Result<T> result) : m_result{ std::move(result) } {}
        void update() override {}
        void cancel() override {}
        const Optional<api::Result<T>>& result() const noexcept override {
            return m_result;
        }

    private:
        Optional<api::Result<T>> m_result;
    };

    void Write(const FilePathView path, const std::string& bytes) {
        REQUIRE(FileSystem::CreateDirectories(FileSystem::ParentPath(path)));
        BinaryWriter writer{ path };
        REQUIRE(writer);
        REQUIRE(writer.write(bytes.data(), bytes.size()) == static_cast<int64>(bytes.size()));
    }

    std::string Read(const FilePathView path) {
        BinaryReader reader{ path };
        REQUIRE(reader);
        std::string bytes(static_cast<std::size_t>(reader.size()), '\0');
        REQUIRE(reader.read(bytes.data(), bytes.size()) == static_cast<int64>(bytes.size()));
        return bytes;
    }

    class Client final : public api::IClient {
    public:
        HashTable<URL, std::string> files;
        Array<URL> downloads;

        api::Request<bool> checkHealth() override {
            return nullptr;
        }
        api::Request<api::User> findUserByCard(StringView) override {
            return nullptr;
        }
        api::Request<Array<api::UserRecord>> fetchUserRecords(StringView) override {
            return nullptr;
        }
        api::Request<Array<api::UserRecord>>
        submitPlayRecords(StringView, const Array<api::RecordSubmission>&) override {
            return nullptr;
        }
        api::Request<api::PlayOptions> fetchPlayOptions(StringView) override {
            return nullptr;
        }
        api::Request<api::PlayOptions> savePlayOptions(StringView, const api::PlayOptions&) override {
            return nullptr;
        }
        api::Request<uint32> incrementCredits(StringView) override {
            return nullptr;
        }
        api::Request<Array<api::Music>> fetchCatalog() override {
            return nullptr;
        }

        api::Request<FilePath> downloadAsset(const URLView url, const FilePathView destination) override {
            downloads.push_back(String{ url });
            Write(destination, files.at(String{ url }));
            return std::make_unique<CompletedRequest<FilePath>>(FilePath{ destination });
        }
    };

    struct Fixture {
        FilePath root = FileSystem::TemporaryDirectoryPath() + UUIDValue::Generate().str() + U"/";
        Client client;
        Array<api::Music> catalog;

        Fixture() {
            api::Music music;
            music.id = U"music";
            music.title = U"Test";
            music.artist = U"Test";
            music.bpm = 120;
            music.genre = U"ORIGINAL";
            music.audio = asset("audio");
            catalog.push_back(std::move(music));
        }

        ~Fixture() {
            FileSystem::Remove(root);
        }

        api::Asset asset(const std::string& bytes) {
            xlair::infra::filesystem::SHA256Hasher hash;
            hash.update({ reinterpret_cast<const uint8*>(bytes.data()), bytes.size() });
            const URL url = U"/musics/" + Unicode::FromUTF8(hash.finish()) + U".wav";
            client.files[url] = bytes;
            return { .url = url, .updated_at = U"now" };
        }

        FilePath audioPath() const {
            return root + catalog.front().audio->url.substr(1);
        }

        api::Request<bool>
        request(const Mode mode = Mode::Incremental, const URLView endpoint = U"https://example.com") {
            return xlair::infra::api::SyncCatalog(client, catalog, endpoint, root, mode);
        }

        void sync(const Mode mode = Mode::Incremental) {
            auto sync_request = request(mode);
            finish(sync_request);
            const auto* error = std::get_if<api::ApiError>(&*sync_request->result());
            INFO(error ? error->message.toUTF8() : "");
            REQUIRE(std::holds_alternative<bool>(*sync_request->result()));
            REQUIRE(std::get<bool>(*sync_request->result()));
        }

        static void finish(api::Request<bool>& request) {
            for (int32 step = 0; step < 100 && !request->result(); ++step) {
                request->update();
            }
            REQUIRE(request->result().has_value());
        }
    };
}

TEST_CASE("Incremental sync reuses verified manifest assets without hashing their contents", "[Sync]") {
    Fixture fixture;
    fixture.sync();
    REQUIRE(fixture.client.downloads.size() == 1);
    fixture.client.downloads.clear();

    // A cached asset is deliberately not rehashed, even if its contents were edited outside sync.
    Write(fixture.audioPath(), "edited");
    auto request = fixture.request();
    request->update();
    REQUIRE(request->result().has_value());
    REQUIRE(std::holds_alternative<bool>(*request->result()));
    CHECK(fixture.client.downloads.isEmpty());
    CHECK(Read(fixture.audioPath()) == "edited");
}

TEST_CASE("Full verification detects and repairs a corrupted cached asset", "[Sync]") {
    Fixture fixture;
    fixture.sync();
    fixture.client.downloads.clear();
    Write(fixture.audioPath(), "edited");
    fixture.sync(Mode::VerifyAll);
    REQUIRE(fixture.client.downloads.size() == 1);
    CHECK(Read(fixture.audioPath()) == "audio");
}

TEST_CASE("Full verification keeps valid assets without downloading them", "[Sync]") {
    Fixture fixture;
    fixture.sync();
    fixture.client.downloads.clear();
    fixture.sync(Mode::VerifyAll);
    CHECK(fixture.client.downloads.isEmpty());
}

TEST_CASE("A missing cached file is downloaded by either sync mode", "[Sync]") {
    Fixture fixture;
    fixture.sync();
    fixture.client.downloads.clear();
    REQUIRE(FileSystem::Remove(fixture.audioPath()));
    const auto mode = GENERATE(Mode::Incremental, Mode::VerifyAll);
    fixture.sync(mode);
    REQUIRE(fixture.client.downloads.size() == 1);
    CHECK(Read(fixture.audioPath()) == "audio");
}

TEST_CASE("Changed remote hashes download new assets and retire obsolete files", "[Sync]") {
    Fixture fixture;
    fixture.sync();
    fixture.client.downloads.clear();
    const auto old_path = fixture.audioPath();
    fixture.catalog.front().audio = fixture.asset("new audio");
    fixture.sync();
    REQUIRE(fixture.client.downloads.size() == 1);
    CHECK(Read(fixture.audioPath()) == "new audio");
    CHECK_FALSE(FileSystem::Exists(old_path));
}

TEST_CASE("Existing assets without a successful manifest must be verified", "[Sync]") {
    Fixture fixture;
    SECTION("A valid untracked file is verified and reused") {
        Write(fixture.audioPath(), "audio");
        auto request = fixture.request();
        request->update();
        CHECK_FALSE(request->result().has_value());
        Fixture::finish(request);
        REQUIRE(std::holds_alternative<bool>(*request->result()));
        CHECK(fixture.client.downloads.isEmpty());
    }
    SECTION("An invalid untracked file is replaced") {
        Write(fixture.audioPath(), "edited");
        fixture.sync();
        CHECK(fixture.client.downloads.size() == 1);
        CHECK(Read(fixture.audioPath()) == "audio");
    }
}

TEST_CASE("Downloaded bytes must match the remote hash before publishing a manifest", "[Sync]") {
    Fixture fixture;
    fixture.client.files[fixture.catalog.front().audio->url] = "wrong download";
    auto request = fixture.request();
    Fixture::finish(request);
    REQUIRE(std::holds_alternative<api::ApiError>(*request->result()));
    CHECK_FALSE(FileSystem::Exists(fixture.root + U"manifest.json"));
    CHECK_FALSE(FileSystem::Exists(fixture.audioPath()));
    CHECK(FileSystem::DirectoryContents(fixture.root, Recursive::Yes).all([](const FilePath& path) {
        return !path.ends_with(U".part");
    }));
}

TEST_CASE("Manifest cache entries cannot be reused across API endpoints", "[Sync]") {
    Fixture fixture;
    fixture.sync();
    fixture.client.downloads.clear();
    auto request = fixture.request(Mode::Incremental, U"https://other.example.com");
    Fixture::finish(request);
    REQUIRE(std::holds_alternative<api::ApiError>(*request->result()));
    CHECK(fixture.client.downloads.isEmpty());
    CHECK(JSON::Load(fixture.root + U"manifest.json")[U"endpoint"].get<String>() == U"https://example.com");
}

TEST_CASE("Deletion journal entries do not count as verified active assets", "[Sync]") {
    Fixture fixture;
    fixture.sync();
    fixture.client.downloads.clear();
    auto manifest = JSON::Load(fixture.root + U"manifest.json");
    const String relative = fixture.catalog.front().audio->url.substr(1);
    manifest[U"musics"][0][U"audio"] = JSON::Parse(U"null");
    manifest[U"pendingDeletes"] = JSON::Parse(U"[]");
    manifest[U"pendingDeletes"].push_back(relative);
    REQUIRE(manifest.save(fixture.root + U"manifest.json"));
    Write(fixture.audioPath(), "edited");
    fixture.sync();
    CHECK(fixture.client.downloads.size() == 1);
    CHECK(Read(fixture.audioPath()) == "audio");
}
