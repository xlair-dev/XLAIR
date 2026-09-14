#include "LocalCatalogSync.hpp"
#include "infra/filesystem/FileHash.hpp"
#include <filesystem>

namespace xlair::infra::api {
    using namespace s3d;
    namespace remote = xlair::api;

    namespace {
        struct Asset {
            URL url;
            String hash;
            FilePath relative;
        };

        bool IsSafeAssetPath(URLView url) {
            if (!url.starts_with(U"/musics/") && !url.starts_with(U"/sheets/")) {
                return false;
            }
            const auto parts = String{ url.substr(1) }.split(U'/');
            return parts.all([](const String& part) {
                return !part.isEmpty() && part != U"." && part != U".." && part.all([](char32_t ch) {
                    return IsAlnum(ch) || ch == U'-' || ch == U'_' || ch == U'.';
                });
            });
        }

        Optional<Asset> ParseAsset(URLView url) {
            if (!IsSafeAssetPath(url)) {
                return none;
            }

            const auto filename = FileSystem::FileName(url);
            const auto hash = FileSystem::BaseName(filename);
            const auto extension = FileSystem::Extension(filename);

            if (hash.size() != 64 || !hash.all([](char32_t ch) {
                    return IsXdigit(ch) && !IsUpper(ch);
                })) {
                return none;
            }
            if (extension.isEmpty() || !extension.all(IsAlnum)) {
                return none;
            }

            return Asset{ String{ url }, hash, String{ url.substr(1) } };
        }

        bool Replace(FilePathView from, FilePathView to) {
            if (FileSystem::Exists(to) && !FileSystem::Remove(to)) {
                return false;
            }
            return FileSystem::Rename(from, to);
        }

        bool IsManagedPath(const String& path) {
            if (path.starts_with(U"assets/")) {
                const auto filename = path.substr(7);
                return !filename.contains(U'/') && ParseAsset(U"/musics/" + filename).has_value();
            }
            return ParseAsset(U"/" + path).has_value();
        }

        class Sync final : public remote::IRequest<bool> {
        public:
            Sync(remote::IClient& client, const Array<remote::Music>& catalog, FilePathView directory, URLView endpoint)
                : m_client{ client } {
                String server{ endpoint };
                while (server.ends_with(U'/')) {
                    server.pop_back();
                }
                m_root = FileSystem::PathAppend(directory, U"sync/");
                m_manifest[U"version"] = 2;
                m_manifest[U"endpoint"] = server;
                m_manifest[U"musics"] = JSON::Parse(U"[]");
                for (const auto& music : catalog) {
                    JSON entry;
                    entry[U"id"] = music.id;
                    entry[U"title"] = music.title;
                    entry[U"artist"] = music.artist;
                    entry[U"bpm"] = music.bpm;
                    entry[U"genre"] = music.genre;
                    entry[U"registrationDate"] = music.registration_date;
                    entry[U"isTest"] = music.is_test;
                    entry[U"jacket"] = add(music.jacket);
                    entry[U"audio"] = add(music.audio);
                    entry[U"sheets"] = JSON::Parse(U"[]");
                    for (const auto& sheet : music.sheets) {
                        JSON value;
                        value[U"id"] = sheet.id;
                        value[U"musicId"] = sheet.music_id;
                        value[U"difficulty"] = sheet.difficulty;
                        value[U"level"] = sheet.level;
                        value[U"notesDesigner"] = sheet.notes_designer;
                        value[U"chart"] = add(sheet.chart);
                        entry[U"sheets"].push_back(value);
                    }
                    m_manifest[U"musics"].push_back(entry);
                }
            }

            ~Sync() override {
                cleanup();
            }

            void cancel() override {
                if (m_result) {
                    return;
                }
                cleanup();
                m_result = remote::ApiError{ remote::ErrorKind::Cancelled, U"Local sync cancelled.", none };
            }

            const Optional<remote::Result<bool>>& result() const noexcept override {
                return m_result;
            }

            void update() override {
                if (m_result) {
                    return;
                }
                try {
                    step();
                } catch (const Error&) {
                    fail(U"Local catalog sync failed.");
                } catch (const std::exception&) {
                    fail(U"Local catalog filesystem operation failed.");
                }
            }

        private:
            JSON add(const Optional<remote::Asset>& asset) {
                if (!asset) {
                    return JSON::Parse(U"null");
                }
                const auto parsed = ParseAsset(asset->url);
                if (!parsed) {
                    fail(U"Invalid catalog asset path or hash.");
                    return JSON::Parse(U"null");
                }
                if (!m_assets.any([&](const Asset& other) {
                        return other.relative == parsed->relative;
                    })) {
                    m_assets.push_back(*parsed);
                }
                JSON result;
                result[U"url"] = asset->url;
                result[U"updatedAt"] = asset->updated_at;
                result[U"path"] = parsed->relative;
                return result;
            }

            void fail(String message) {
                if (m_index < m_assets.size()) {
                    message += U"\n" + m_assets[m_index].url;
                }
                m_result = remote::ApiError{ remote::ErrorKind::InvalidResponse, std::move(message), none };
                cleanup();
            }
            void cleanup() {
                if (m_request) {
                    m_request->cancel();
                    m_request.reset();
                }
                m_reader.close();
                if (!m_partial.isEmpty()) {
                    FileSystem::Remove(m_partial);
                    m_partial.clear();
                }
            }
            void startHash(FilePathView path) {
                m_hash = filesystem::SHA256Hasher{};
                if (!m_reader.open(path)) {
                    fail(U"Could not open local asset for verification.");
                }
            }
            void download() {
                const auto path = FileSystem::PathAppend(m_root, m_assets[m_index].relative);
                if (!FileSystem::CreateDirectories(FileSystem::ParentPath(path))) {
                    fail(U"Could not create asset directory.");
                    return;
                }
                m_partial = path + U"." + UUIDValue::Generate().str() + U".part";
                m_request = m_client.downloadAsset(m_assets[m_index].url, m_partial);
                if (!m_request) {
                    fail(U"The API client did not create an asset request.");
                }
            }
            // Persist the deletion journal in the new manifest before deleting anything.
            bool prepareDeletions() {
                HashSet<String> candidates;
                const auto path = m_root + U"manifest.json";
                if (FileSystem::Exists(path)) {
                    const auto old = JSON::Load(path);
                    if (!old || !old.isObject() || !old[U"musics"].isArray() ||
                        old[U"endpoint"].getOpt<String>() != m_manifest[U"endpoint"].getOpt<String>()) {
                        fail(U"Cannot read the previous sync manifest safely.");
                        return false;
                    }
                    const auto collect = [&](const JSON& asset) {
                        if (asset.isNull()) {
                            return;
                        }
                        const auto relative = asset[U"path"].getOpt<String>();
                        if (!relative || !IsManagedPath(*relative)) {
                            throw Error{ U"Invalid managed asset path." };
                        }
                        candidates.insert(*relative);
                    };
                    for (const auto& music : old[U"musics"]) {
                        collect(music.value[U"jacket"]);
                        collect(music.value[U"audio"]);
                        for (const auto& sheet : music.value[U"sheets"]) {
                            collect(sheet.value[U"chart"]);
                        }
                    }
                    if (old.hasElement(U"pendingDeletes")) {
                        if (!old[U"pendingDeletes"].isArray()) {
                            throw Error{ U"Invalid deletion journal." };
                        }
                        for (const auto& item : old[U"pendingDeletes"]) {
                            const auto relative = item.value.getOpt<String>();
                            if (!relative || !IsManagedPath(*relative)) {
                                throw Error{ U"Invalid deletion path." };
                            }
                            candidates.insert(*relative);
                        }
                    }
                }
                for (const auto& asset : m_assets) {
                    candidates.erase(asset.relative);
                }
                m_manifest[U"pendingDeletes"] = JSON::Parse(U"[]");
                for (const auto& relative : candidates) {
                    const std::filesystem::path path{ (m_root + relative).toUTF32() };
                    // Do not discard failures (including symlinks); only retire confirmed missing paths.
                    std::error_code error;
                    const auto status = std::filesystem::symlink_status(path, error);
                    if (status.type() == std::filesystem::file_type::not_found) {
                        continue;
                    }
                    m_manifest[U"pendingDeletes"].push_back(relative);
                }
                return true;
            }

            void deleteObsolete() {
                auto root = std::filesystem::path{ std::u32string{ m_root.begin(), m_root.end() } };
                if (!root.has_filename()) {
                    root = root.parent_path();
                }
                for (const auto& item : m_manifest[U"pendingDeletes"]) {
                    const auto relative = item.value.get<String>();
                    std::error_code error;
                    auto path = root;
                    bool safe = !std::filesystem::is_symlink(std::filesystem::symlink_status(root, error));
                    for (const auto& part : relative.split(U'/')) {
                        path /= std::filesystem::path{ std::u32string{ part.begin(), part.end() } };
                        error.clear();
                        const auto status = std::filesystem::symlink_status(path, error);
                        if (std::filesystem::is_symlink(status) ||
                            (error && error != std::errc::no_such_file_or_directory)) {
                            safe = false;
                        }
                    }
                    error.clear();
                    if (!safe || std::filesystem::is_directory(path, error)) {
                        Logger << U"[Sync] Skipped unsafe deletion: " + relative;
                        continue;
                    }
                    error.clear();
                    std::filesystem::remove(path, error); // A missing file is already cleaned up.
                    if (error) {
                        Logger << U"[Sync] Could not delete obsolete asset: " + relative;
                        continue;
                    }
                    // Never recursively delete directories: leave any untracked files untouched.
                    for (auto parent = path.parent_path(); parent != root && parent.has_relative_path();
                         parent = parent.parent_path()) {
                        error.clear();
                        if (!std::filesystem::is_empty(parent, error) || error) {
                            break;
                        }
                        std::filesystem::remove(parent, error);
                        if (error) {
                            break;
                        }
                    }
                }
                // Keep the journal until the next successful sync; replay is idempotent,
                // including after a crash between manifest publication and deletion.
            }
            void step() {
                if (!m_endpoint_checked) {
                    const auto path = m_root + U"manifest.json";
                    if (FileSystem::Exists(path)) {
                        const auto previous = JSON::Load(path);
                        const auto endpoint = previous.isObject() ? previous[U"endpoint"].getOpt<String>() : none;
                        if (!endpoint) {
                            fail(U"Cannot read the previous sync endpoint safely.");
                            return;
                        }
                        if (*endpoint != m_manifest[U"endpoint"].get<String>()) {
                            m_result = remote::ApiError{
                                remote::ErrorKind::Configuration,
                                U"The sync directory belongs to a different API endpoint. Explicitly "
                                U"reset or move the sync directory before switching servers.",
                                none,
                            };
                            return;
                        }
                    }
                    m_endpoint_checked = true;
                }
                if (m_index == m_assets.size()) {
                    if (!FileSystem::CreateDirectories(m_root)) {
                        fail(U"Could not create sync directory.");
                        return;
                    }
                    if (!prepareDeletions()) {
                        return;
                    }
                    m_partial = m_root + U"manifest." + UUIDValue::Generate().str() + U".part";
                    if (!m_manifest.save(m_partial) || !Replace(m_partial, m_root + U"manifest.json")) {
                        fail(U"Could not publish local catalog manifest.");
                        return;
                    }
                    m_partial.clear();
                    try {
                        deleteObsolete();
                    } catch (...) {
                        Logger << U"[Sync] Cleanup incomplete; it will be retried on the next sync.";
                    }
                    m_result = true;
                    return;
                }
                if (m_request) {
                    m_request->update();
                    if (!m_request->result()) {
                        return;
                    }
                    if (const auto* error = std::get_if<remote::ApiError>(&*m_request->result())) {
                        m_result = *error;
                        std::get<remote::ApiError>(*m_result).message += U"\n" + m_assets[m_index].url;
                        cleanup();
                        return;
                    }
                    m_request.reset();
                    startHash(m_partial);
                    return;
                }
                const auto path = FileSystem::PathAppend(m_root, m_assets[m_index].relative);
                if (!m_reader) {
                    if (FileSystem::IsFile(path)) {
                        startHash(path);
                    } else {
                        download();
                    }
                    return;
                }
                // Bound disk reads and hashing per frame; do not load entire audio files into memory.
                std::array<uint8, 65536> buffer;
                const auto count = m_reader.read(buffer.data(), buffer.size());
                if (count > 0) {
                    m_hash.update({ buffer.data(), static_cast<std::size_t>(count) });
                    return;
                }
                m_reader.close();
                if (Unicode::FromUTF8(m_hash.finish()) != m_assets[m_index].hash) {
                    if (m_partial.isEmpty()) {
                        download();
                    } else {
                        fail(U"Downloaded asset SHA-256 does not match its filename.");
                    }
                    return;
                }
                if (!m_partial.isEmpty()) {
                    if (!Replace(m_partial, path)) {
                        fail(U"Could not publish downloaded asset.");
                        return;
                    }
                    m_partial.clear();
                }
                ++m_index;
            }
            remote::IClient& m_client;
            FilePath m_root, m_partial;
            bool m_endpoint_checked = false;
            JSON m_manifest;
            Array<Asset> m_assets;
            std::size_t m_index = 0;
            remote::Request<FilePath> m_request;
            BinaryReader m_reader;
            filesystem::SHA256Hasher m_hash;
            Optional<remote::Result<bool>> m_result;
        };
    }
    remote::Request<bool> SyncCatalog(
        remote::IClient& client,
        const Array<remote::Music>& catalog,
        FilePathView directory,
        URLView endpoint
    ) {
        return std::make_unique<Sync>(client, catalog, directory, endpoint);
    }
}
