#include "Loader.hpp"

#include <Siv3D/JSON.hpp>
#include <Siv3D/TOMLReader.hpp>

#include <cmath>
#include <limits>

namespace xlair::sheets::metadata {
    namespace {
        constexpr s3d::int64 SupportedVersion = 1;

        [[nodiscard]] s3d::FilePath ResolvePath(const s3d::FilePath& source_path,
                                                const s3d::FilePathView referenced_path) {
            if (referenced_path.isEmpty()) {
                return U"";
            }

            return s3d::FileSystem::FullPath(
                s3d::FileSystem::PathAppend(s3d::FileSystem::ParentPath(source_path), referenced_path));
        }

        [[nodiscard]] s3d::String JsonStringOr(const s3d::JSON& object, const s3d::StringView key,
                                               const s3d::StringView default_value = U"") {
            return object.hasElement(key) ? object[key].getOr<s3d::String>(s3d::String{ default_value })
                                          : s3d::String{ default_value };
        }

        [[nodiscard]] double JsonDoubleOr(const s3d::JSON& object, const s3d::StringView key,
                                          const double default_value) {
            return object.hasElement(key) ? object[key].getOr<double>(default_value) : default_value;
        }

        [[nodiscard]] s3d::String TomlStringOr(const s3d::TOMLValue& object, const s3d::String& key,
                                               const s3d::StringView default_value = U"") {
            return object.hasMember(key) ? object[key].getOr<s3d::String>(s3d::String{ default_value })
                                         : s3d::String{ default_value };
        }

        [[nodiscard]] double TomlDoubleOr(const s3d::TOMLValue& object, const s3d::String& key,
                                          const double default_value) {
            return object.hasMember(key) ? object[key].getOr<double>(default_value) : default_value;
        }

        [[nodiscard]] s3d::String JsonId(const s3d::JSON& value) {
            if (value.isString()) {
                return value.getOr<s3d::String>(U"");
            }
            if (value.isInteger()) {
                return s3d::Format(value.getOr<s3d::int64>(0));
            }
            return U"";
        }

        [[nodiscard]] s3d::String TomlId(const s3d::TOMLValue& value) {
            if (value.isString()) {
                return value.getOr<s3d::String>(U"");
            }
            if (value.isNumber()) {
                return s3d::Format(value.getOr<s3d::int64>(0));
            }
            return U"";
        }

        [[nodiscard]] s3d::Optional<s3d::uint32> DifficultyIndex(const s3d::int64 index) {
            if (index < 0 || index > static_cast<s3d::int64>(std::numeric_limits<s3d::uint32>::max())) {
                return s3d::none;
            }
            return static_cast<s3d::uint32>(index);
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
            metadata.id = JsonStringOr(json, U"id");
            metadata.title = JsonStringOr(json, U"title", metadata.title);
            metadata.title_sort = JsonStringOr(json, U"title_sort", metadata.title_sort);
            metadata.artist = JsonStringOr(json, U"artist", metadata.artist);
            metadata.genre = JsonStringOr(json, U"genre", metadata.genre);
            metadata.music = ResolvePath(metadata.source_path, JsonStringOr(json, U"music"));
            metadata.jacket = ResolvePath(metadata.source_path, JsonStringOr(json, U"jacket"));
            metadata.url = JsonStringOr(json, U"url");
            metadata.music_offset_seconds = JsonDoubleOr(json, U"music_offset", metadata.music_offset_seconds);
            metadata.demo_start_seconds = JsonDoubleOr(json, U"demo_start", metadata.demo_start_seconds);
            metadata.bpm = JsonDoubleOr(json, U"bpm", metadata.bpm);

            if (json.hasElement(U"difficulties")) {
                const auto difficulties = json[U"difficulties"];
                if (!difficulties.isArray()) {
                    return Result<Metadata>::makeError(U"Metadata difficulties must be an array.", path);
                }

                for (const auto& [_array_index, object] : difficulties) {
                    if (!object.isObject()) {
                        return Result<Metadata>::makeError(U"Each metadata difficulty must be an object.", path);
                    }
                    if (!object.hasElement(U"id")) {
                        continue;
                    }

                    const s3d::StringView index_key = object.hasElement(U"index") ? U"index" : U"difficulty";
                    const auto index = DifficultyIndex(object[index_key].getOr<s3d::int64>(0));
                    if (!index) {
                        return Result<Metadata>::makeError(U"Difficulty indices must fit in the uint32 range.", path);
                    }

                    const s3d::FilePath chart =
                        object.hasElement(U"chart") ? JsonStringOr(object, U"chart") : JsonStringOr(object, U"src");
                    metadata.difficulties.push_back({
                        .index = *index,
                        .level = JsonDoubleOr(object, U"level", 0.0),
                        .id = JsonId(object[U"id"]),
                        .chart = ResolvePath(metadata.source_path, chart),
                        .designer = JsonStringOr(object, U"designer", U"Anonymous"),
                    });
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
            metadata.id = TomlStringOr(toml, U"id");
            metadata.title = TomlStringOr(toml, U"title", metadata.title);
            metadata.title_sort = TomlStringOr(toml, U"title_sort", metadata.title_sort);
            metadata.artist = TomlStringOr(toml, U"artist", metadata.artist);
            metadata.genre = TomlStringOr(toml, U"genre", metadata.genre);
            metadata.music = ResolvePath(metadata.source_path, TomlStringOr(toml, U"music"));
            metadata.jacket = ResolvePath(metadata.source_path, TomlStringOr(toml, U"jacket"));
            metadata.url = TomlStringOr(toml, U"url");
            metadata.music_offset_seconds = TomlDoubleOr(toml, U"music_offset", metadata.music_offset_seconds);
            metadata.demo_start_seconds = TomlDoubleOr(toml, U"demo_start", metadata.demo_start_seconds);
            metadata.bpm = TomlDoubleOr(toml, U"bpm", metadata.bpm);

            if (toml.hasMember(U"difficulties")) {
                const auto difficulties = toml[U"difficulties"];
                if (!difficulties.isTableArray()) {
                    return Result<Metadata>::makeError(U"Metadata difficulties must be an array of tables.", path);
                }

                for (const auto& object : difficulties.tableArrayView()) {
                    if (!object.hasMember(U"id")) {
                        continue;
                    }

                    const s3d::String index_key = object.hasMember(U"index") ? U"index" : U"difficulty";
                    const auto index = DifficultyIndex(object[index_key].getOr<s3d::int64>(0));
                    if (!index) {
                        return Result<Metadata>::makeError(U"Difficulty indices must fit in the uint32 range.", path);
                    }

                    const s3d::FilePath chart =
                        object.hasMember(U"chart") ? TomlStringOr(object, U"chart") : TomlStringOr(object, U"src");
                    metadata.difficulties.push_back({
                        .index = *index,
                        .level = TomlDoubleOr(object, U"level", 0.0),
                        .id = TomlId(object[U"id"]),
                        .chart = ResolvePath(metadata.source_path, chart),
                        .designer = TomlStringOr(object, U"designer", U"Anonymous"),
                    });
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

        auto paths = s3d::FileSystem::DirectoryContents(directory, s3d::Recursive::Yes);
        paths.sort();

        Result<s3d::Array<Metadata>> result;
        s3d::Array<Metadata> metadata_list;
        for (const auto& path : paths) {
            if (!s3d::FileSystem::IsFile(path)) {
                continue;
            }

            const auto file_name = s3d::FileSystem::FileName(path).lowercased();
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
