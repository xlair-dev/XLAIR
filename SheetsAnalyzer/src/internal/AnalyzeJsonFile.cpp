#include "internal/AnalyzeFile.hpp"

namespace SheetsAnalyzer::internal {

    template<typename T>
    void helper(const s3d::JSON& json, const s3d::String& key, T& entry, const auto& default_value) {
        if (json.hasElement(key)) {
            entry = json[key].getOr<T>(default_value);
        } else {
            entry = default_value;
        }
    }

    s3d::Optional<Metadata> AnalyzeJsonFile(const s3d::FilePath& path) {
        const auto json = s3d::JSON::Load(path);
        if (not json) {
            throw s3d::Error { U"Failed to load JSON file: " + path };
        }

        if (not json.hasElement(U"version")) {
            return s3d::none; // No version specified, return empty Optional
        }
        const auto version = json[U"version"].getOr<s3d::int32>(-1);
        if (version != Constant::Version) {
            return s3d::none; // Unsupported version
        }

        Metadata metadata;
        helper(json, U"id", metadata.id, U"");
        helper(json, U"title", metadata.title, Constant::DefaultTitle);
        helper(json, U"title_sort", metadata.title_sort, Constant::DefaultTitleSort);
        helper(json, U"artist", metadata.artist, Constant::DefaultArtist);
        helper(json, U"genre", metadata.genre, Constant::DefaltGenre);
        helper(json, U"music", metadata.music, U"");
        helper(json, U"jacket", metadata.jacket, U"");
        helper(json, U"url", metadata.url, s3d::URL {});
        helper(json, U"music_offset", metadata.music_offset, Constant::DefaultMusicOffset);
        helper(json, U"bpm", metadata.bpm, Constant::DefaultBPM);

        if (json.hasElement(U"difficulties")) {
            const auto& difficulties = json[U"difficulties"];
            if (difficulties.isArray()) {
                for (const auto& [index, object] : difficulties) {
                    Difficulty difficulty;
                    if (not object.hasElement(U"id")) {
                        continue;
                    }
                    const auto id = object[U"id"].getOr<s3d::uint32>(0);
                    if (id >= Constant::MaxDifficulties) {
                        continue;
                    }
                    helper(object, U"level", difficulty.level, 0.0);
                    helper(object, U"src", difficulty.src, U"");
                    helper(object, U"desiner", difficulty.desiner, Constant::DefaultDesiner);
                    metadata.difficulties[id] = difficulty;
                }
            }
        }
        return metadata;
    }
}
