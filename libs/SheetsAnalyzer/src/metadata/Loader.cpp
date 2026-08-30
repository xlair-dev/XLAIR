#include "Loader.hpp"

#include <Siv3D/JSON.hpp>
#include <Siv3D/TOMLReader.hpp>

#include <cmath>
#include <utility>

namespace xlair::sheets::metadata {
    namespace {
        constexpr s3d::int64 SupportedVersion = 1;

        [[nodiscard]] inline s3d::FilePath ResolvePath(const s3d::FilePathView& source_path,
                                                       const s3d::FilePathView referenced_path) {
            if (referenced_path.isEmpty()) {
                return U"";
            }

            const bool is_content_root_relative = referenced_path.starts_with(U'/');
            const s3d::FilePath base_path = is_content_root_relative ? s3d::FileSystem::CurrentDirectory()
                                                                     : s3d::FileSystem::ParentPath(source_path);
            const s3d::FilePathView relative_path =
                is_content_root_relative ? referenced_path.substr(1) : referenced_path;
            return s3d::FileSystem::FullPath(s3d::FileSystem::PathAppend(base_path, relative_path));
        }

        template <class T> inline void ReadValue(const s3d::JSON& object, const s3d::String& key, T& entry) {
            if (object.hasElement(key)) {
                if (auto value = object[key].getOpt<T>()) {
                    entry = std::move(*value);
                }
            }
        }

        template <class T> inline void ReadValue(const s3d::TOMLValue& object, const s3d::String& key, T& entry) {
            if (object.hasMember(key)) {
                if (auto value = object[key].getOpt<T>()) {
                    entry = std::move(*value);
                }
            }
        }

        void StoreDifficulty(s3d::Array<Difficulty>& difficulties, Difficulty difficulty) {
            for (auto& existing : difficulties) {
                if (existing.index == difficulty.index) {
                    existing = std::move(difficulty);
                    return;
                }
            }

            difficulties.push_back(std::move(difficulty));
        }

        [[nodiscard]] Result<Metadata> Validate(Metadata metadata, const s3d::FilePath& path) {
            if (!std::isfinite(metadata.music_offset_seconds) || !std::isfinite(metadata.demo_start_seconds)) {
                return Result<Metadata>::makeError(U"Metadata time offsets must be finite numbers.", path);
            }
            if (!std::isfinite(metadata.bpm) || metadata.bpm <= 0.0) {
                return Result<Metadata>::makeError(U"Metadata BPM must be a positive finite number.", path);
            }
            for (const auto& difficulty : metadata.difficulties) {
                if (!std::isfinite(difficulty.level) || difficulty.level < 0.0) {
                    return Result<Metadata>::makeError(U"Difficulty levels must be non-negative finite numbers.", path);
                }
            }
            return Result<Metadata>{ std::move(metadata) };
        }

        [[nodiscard]] Result<Metadata> LoadJson(const s3d::FilePath& path) {
            const s3d::JSON json = s3d::JSON::Load(path);
            if (!json) {
                return Result<Metadata>::makeError(U"Failed to read JSON metadata.", path);
            }
            if (!json.isObject()) {
                return Result<Metadata>::makeError(U"JSON metadata must contain an object at its root.", path);
            }

            const auto version = json.hasElement(U"version") ? json[U"version"].getOpt<s3d::int64>() : s3d::none;
            if (!version || *version != SupportedVersion) {
                return Result<Metadata>::makeError(U"Metadata must use supported version 1.", path);
            }

            Metadata metadata;
            metadata.source_path = s3d::FileSystem::FullPath(path);
            ReadValue(json, U"id", metadata.id);
            ReadValue(json, U"title", metadata.title);
            ReadValue(json, U"title_sort", metadata.title_sort);
            ReadValue(json, U"artist", metadata.artist);
            ReadValue(json, U"genre", metadata.genre);
            ReadValue(json, U"url", metadata.url);
            ReadValue(json, U"music_offset", metadata.music_offset_seconds);
            ReadValue(json, U"demo_start", metadata.demo_start_seconds);
            ReadValue(json, U"bpm", metadata.bpm);

            s3d::FilePath music;
            s3d::FilePath jacket;
            ReadValue(json, U"music", music);
            ReadValue(json, U"jacket", jacket);
            metadata.music = ResolvePath(metadata.source_path, music);
            metadata.jacket = ResolvePath(metadata.source_path, jacket);

            if (json.hasElement(U"difficulties")) {
                const auto difficulties = json[U"difficulties"];
                if (difficulties.isArray()) {
                    for (const auto& [_array_index, object] : difficulties) {
                        if (!object.hasElement(U"id")) {
                            continue;
                        }

                        Difficulty difficulty;
                        ReadValue(object, U"difficulty", difficulty.index);
                        ReadValue(object, U"id", difficulty.id);
                        ReadValue(object, U"level", difficulty.level);
                        ReadValue(object, U"designer", difficulty.designer);

                        s3d::FilePath chart;
                        ReadValue(object, U"src", chart);
                        difficulty.chart = ResolvePath(metadata.source_path, chart);
                        StoreDifficulty(metadata.difficulties, std::move(difficulty));
                    }
                }
            }

            metadata.difficulties.stable_sort_by([](const auto& left, const auto& right) {
                return left.index < right.index;
            });
            return Validate(std::move(metadata), path);
        }

        [[nodiscard]] Result<Metadata> LoadToml(const s3d::FilePath& path) {
            const s3d::TOMLReader toml{ path };
            if (!toml) {
                return Result<Metadata>::makeError(U"Failed to read TOML metadata.", path);
            }

            const auto version = toml.hasMember(U"version") ? toml[U"version"].getOpt<s3d::int64>() : s3d::none;
            if (!version || *version != SupportedVersion) {
                return Result<Metadata>::makeError(U"Metadata must use supported version 1.", path);
            }

            Metadata metadata;
            metadata.source_path = s3d::FileSystem::FullPath(path);
            ReadValue(toml, U"id", metadata.id);
            ReadValue(toml, U"title", metadata.title);
            ReadValue(toml, U"title_sort", metadata.title_sort);
            ReadValue(toml, U"artist", metadata.artist);
            ReadValue(toml, U"genre", metadata.genre);
            ReadValue(toml, U"url", metadata.url);
            ReadValue(toml, U"music_offset", metadata.music_offset_seconds);
            ReadValue(toml, U"demo_start", metadata.demo_start_seconds);
            ReadValue(toml, U"bpm", metadata.bpm);

            s3d::FilePath music;
            s3d::FilePath jacket;
            ReadValue(toml, U"music", music);
            ReadValue(toml, U"jacket", jacket);
            metadata.music = ResolvePath(metadata.source_path, music);
            metadata.jacket = ResolvePath(metadata.source_path, jacket);

            if (toml.hasMember(U"difficulties")) {
                const auto difficulties = toml[U"difficulties"];
                if (difficulties.isTableArray()) {
                    for (const auto& object : difficulties.tableArrayView()) {
                        if (!object.hasMember(U"id")) {
                            continue;
                        }

                        Difficulty difficulty;
                        ReadValue(object, U"difficulty", difficulty.index);
                        ReadValue(object, U"id", difficulty.id);
                        ReadValue(object, U"level", difficulty.level);
                        ReadValue(object, U"designer", difficulty.designer);

                        s3d::FilePath chart;
                        ReadValue(object, U"src", chart);
                        difficulty.chart = ResolvePath(metadata.source_path, chart);
                        StoreDifficulty(metadata.difficulties, std::move(difficulty));
                    }
                }
            }

            metadata.difficulties.stable_sort_by([](const auto& left, const auto& right) {
                return left.index < right.index;
            });
            return Validate(std::move(metadata), path);
        }

        void AppendDiagnostics(s3d::Array<Diagnostic>& destination, const s3d::Array<Diagnostic>& source) {
            for (const auto& diagnostic : source) {
                destination.push_back(diagnostic);
            }
        }
    }

    Result<Metadata> Load(const s3d::FilePath& path) {
        const auto extension = s3d::FileSystem::Extension(path).lowercased();
        if (extension == U"json") {
            return LoadJson(path);
        }
        if (extension == U"toml") {
            return LoadToml(path);
        }
        return Result<Metadata>::makeError(U"Unsupported metadata format: " + extension, path);
    }

    Result<s3d::Array<Metadata>> Scan(const s3d::FilePath& directory) {
        if (!s3d::FileSystem::IsDirectory(directory)) {
            return Result<s3d::Array<Metadata>>::makeError(U"Metadata scan path must be a directory.", directory);
        }

        const auto paths = s3d::FileSystem::DirectoryContents(directory, s3d::Recursive::Yes).sorted();

        Result<s3d::Array<Metadata>> result;
        s3d::Array<Metadata> metadata_list;
        for (const auto& path : paths) {
            if (!s3d::FileSystem::IsFile(path)) {
                continue;
            }

            const auto file_name = s3d::FileSystem::FileName(path);
            if (file_name != U"music.json" && file_name != U"music.toml") {
                continue;
            }

            auto metadata = Load(path);
            AppendDiagnostics(result.diagnostics, metadata.diagnostics);
            if (metadata) {
                metadata_list.push_back(std::move(*metadata));
            }
        }

        if (result.diagnostics.isEmpty()) {
            result.value = std::move(metadata_list);
        }
        return result;
    }
}
