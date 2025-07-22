#include "internal/AnalyzeFile.hpp"

namespace SheetsAnalyzer::internal {
    template<typename T>
    void helper(const s3d::TOMLValue& toml, const s3d::String& key, T& entry, const auto& default_value) {
        if (toml.hasMember(key)) {
            entry = toml[key].getOr<T>(default_value);
        } else {
            entry = default_value;
        }
    }

    s3d::Optional<Metadata> AnalyzeTomlFile(const s3d::FilePath& path) {
        const s3d::TOMLReader toml(path);
        if (not toml) {
            throw s3d::Error { U"Failed to load TOML file: " + path };
        }

        if (not toml.hasMember(U"version")) {
            return s3d::none; // No version specified, return empty Optional
        }
        const auto version = toml[U"version"].getOr<s3d::int32>(-1);
        if (version != Constant::Version) {
            return s3d::none; // Unsupported version
        }

        Metadata metadata;
        metadata.path = s3d::FileSystem::FullPath(path);
        helper(toml, U"id", metadata.id, U"");
        helper(toml, U"title", metadata.title, Constant::DefaultTitle);
        helper(toml, U"title_sort", metadata.title_sort, Constant::DefaultTitleSort);
        helper(toml, U"artist", metadata.artist, Constant::DefaultArtist);
        helper(toml, U"genre", metadata.genre, Constant::DefaltGenre);
        helper(toml, U"music", metadata.music, U"");
        helper(toml, U"jacket", metadata.jacket, U"");
        helper(toml, U"url", metadata.url, s3d::URL {});
        helper(toml, U"music_offset", metadata.music_offset, Constant::DefaultMusicOffset);
        helper(toml, U"bpm", metadata.bpm, Constant::DefaultBPM);

        if (toml.hasMember(U"difficulties")) {
            const auto& difficulties = toml[U"difficulties"];
            if (difficulties.isTableArray()) {
                for (const auto& object : difficulties.tableArrayView()) {
                    Difficulty difficulty;
                    if (not object.hasMember(U"id")) {
                        continue;
                    }
                    const auto id = object[U"id"].getOr<s3d::uint32>(0);
                    if (id >= Constant::MaxDifficulties) {
                        continue;
                    }
                    helper(object, U"level", difficulty.level, 0.0);
                    helper(object, U"src", difficulty.src, U"");
                    helper(object, U"designer", difficulty.designer, Constant::DefaultDesigner);
                    metadata.difficulties[id] = difficulty;
                }
            }
        }
        return metadata;
    }
}
