#include "MusicMetadata.hpp"

namespace xlair::infra::api {
    namespace {
        s3d::String LocalPath(const s3d::JSON& asset) {
            return asset.isNull() ? U"" : U"../../" + asset[U"path"].get<s3d::String>();
        }

        s3d::uint32 DifficultyIndex(const s3d::String& name) {
            if (name == U"basic")
                return 0;
            if (name == U"advanced")
                return 1;
            if (name == U"master")
                return 2;
            throw s3d::Error{ U"Unsupported catalog difficulty." };
        }
    }

    s3d::JSON MakeMusicMetadata(const s3d::JSON& music) {
        s3d::JSON result;
        result[U"version"] = 1;
        for (const auto* key : { U"id", U"title", U"artist", U"genre", U"bpm" }) {
            result[key] = music[key];
        }
        result[U"title_sort"] = music[U"title"];
        result[U"music"] = LocalPath(music[U"audio"]);
        result[U"jacket"] = LocalPath(music[U"jacket"]);
        result[U"music_offset"] = 0.0;
        result[U"demo_start"] = 0.0;
        result[U"difficulties"] = s3d::JSON::Parse(U"[]");
        for (const auto& entry : music[U"sheets"]) {
            const auto& sheet = entry.value;
            if (sheet[U"chart"].isNull()) {
                continue;
            }
            s3d::JSON difficulty;
            difficulty[U"id"] = sheet[U"id"];
            difficulty[U"difficulty"] = DifficultyIndex(sheet[U"difficulty"].get<s3d::String>());
            difficulty[U"level"] = sheet[U"level"];
            difficulty[U"designer"] = sheet[U"notesDesigner"];
            difficulty[U"src"] = LocalPath(sheet[U"chart"]);
            result[U"difficulties"].push_back(difficulty);
        }
        return result;
    }
}
