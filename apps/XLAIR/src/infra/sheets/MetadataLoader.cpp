#include "MetadataLoader.hpp"
#include "infra/filesystem/PathUtils.hpp"

#include <Siv3D/JSON.hpp>

#include <utility>

namespace xlair::infra::sheets {
    MetadataLoader::MetadataLoader(s3d::FilePath local_directory, s3d::FilePath sync_directory)
        : m_local_directory{ std::move(local_directory) }, m_sync_directory{ std::move(sync_directory) } {}

    xlair::sheets::Result<s3d::Array<xlair::sheets::Metadata>> MetadataLoader::load() const {
        using MetadataResult = xlair::sheets::Result<s3d::Array<xlair::sheets::Metadata>>;

        const auto default_sync_directory = s3d::FileSystem::PathAppend(m_local_directory, U"sync/");
        if (filesystem::IsSameOrWithin(m_local_directory, m_sync_directory)) {
            return MetadataResult::makeError(
                U"The sync directory must not contain the local sheet metadata directory.",
                m_sync_directory
            );
        }

        if (!s3d::FileSystem::Exists(m_local_directory) && !s3d::FileSystem::CreateDirectories(m_local_directory)) {
            return MetadataResult::makeError(U"Failed to create the sheet metadata directory.", m_local_directory);
        }

        if (!s3d::FileSystem::Exists(m_sync_directory) && !s3d::FileSystem::CreateDirectories(m_sync_directory)) {
            return MetadataResult::makeError(U"Failed to create the sync directory.", m_sync_directory);
        }

        // Keep older sync roots under data/sheets out of the local catalog after a directory change.
        s3d::Array<s3d::FilePath> excluded_directories{ default_sync_directory, m_sync_directory };
        for (const auto& path : s3d::FileSystem::DirectoryContents(m_local_directory, s3d::Recursive::Yes)) {
            if (!s3d::FileSystem::IsFile(path) || s3d::FileSystem::FileName(path) != U"manifest.json") {
                continue;
            }

            const auto manifest = s3d::JSON::Load(path);
            if (manifest.isObject() && manifest[U"endpoint"].getOpt<s3d::String>() && manifest[U"musics"].isArray()) {
                excluded_directories.push_back(s3d::FileSystem::ParentPath(path));
            }
        }

        auto local = xlair::sheets::ScanMetadata(m_local_directory, excluded_directories);
        if (!local) {
            return local;
        }

        auto synced = xlair::sheets::ScanMetadata(m_sync_directory);
        if (!synced) {
            return synced;
        }

        // Older synced metadata did not retain isTest; its manifest still does.
        s3d::HashTable<s3d::String, bool> test_flags;
        const auto manifest_path = s3d::FileSystem::PathAppend(m_sync_directory, U"manifest.json");
        const auto manifest = s3d::FileSystem::IsFile(manifest_path) ? s3d::JSON::Load(manifest_path) : s3d::JSON{};
        if (manifest.isObject() && manifest.hasElement(U"musics") && manifest[U"musics"].isArray()) {
            for (const auto& entry : manifest[U"musics"]) {
                if (!entry.value.isObject() || !entry.value.hasElement(U"id") || !entry.value.hasElement(U"isTest")) {
                    continue;
                }
                const auto id = entry.value[U"id"].getOpt<s3d::String>();
                const auto is_test = entry.value[U"isTest"].getOpt<bool>();
                if (id && is_test) {
                    test_flags[*id] = *is_test;
                }
            }
        }
        for (auto& metadata : *synced) {
            if (const auto flag = test_flags.find(metadata.id); flag != test_flags.end()) {
                metadata.is_test = flag->second;
            }
            local->push_back(std::move(metadata));
        }
        return local;
    }
}
