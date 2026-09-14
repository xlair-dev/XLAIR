#include "Parser.hpp"

#include <cmath>
#include <concepts>
#include <utility>

namespace xlair::api {
    using namespace s3d::Literals;
    namespace {
        template <class T> T Read(const s3d::JSON& json, s3d::StringView key) {
            if (!json.isObject() || !json.hasElement(key)) {
                throw s3d::Error{ U"Missing API response field: {}"_fmt(key) };
            }
            if constexpr (std::integral<T> && !std::same_as<T, bool>) {
                if (!json[key].isInteger() && !json[key].isUnsigned()) {
                    throw s3d::Error{ U"Expected an integer API response field: {}"_fmt(key) };
                }
                const auto value = json[key].getOpt<s3d::int64>();
                if (value && std::in_range<T>(*value)) {
                    return static_cast<T>(*value);
                }
            } else {
                const auto value = json[key].getOpt<T>();
                if (value) {
                    if constexpr (std::floating_point<T>) {
                        if (!std::isfinite(*value)) {
                            throw s3d::Error{ U"Non-finite API response field: {}"_fmt(key) };
                        }
                    }
                    return *value;
                }
            }
            throw s3d::Error{ U"Invalid API response field: {}"_fmt(key) };
        }

        s3d::Optional<Asset> ReadAsset(const s3d::JSON& json, s3d::StringView key) {
            if (!json.isObject()) {
                throw s3d::Error{ U"Expected an API response object." };
            }
            if (!json.hasElement(key) || json[key].isNull())
                return s3d::none;
            return Asset{
                .url = Read<s3d::URL>(json[key], U"url"),
                .updated_at = Read<s3d::String>(json[key], U"updatedAt"),
            };
        }

        template <class T, class Parser> s3d::Array<T> ReadArray(const s3d::JSON& json, Parser parser) {
            if (!json.isArray()) {
                throw s3d::Error{ U"Expected an API response array." };
            }
            s3d::Array<T> result;
            for (const auto& entry : json) {
                result.push_back(parser(entry.value));
            }
            return result;
        }

        ClearType ParseClearType(const s3d::String& value) {
            if (value == U"failed")
                return ClearType::Failed;
            if (value == U"clear")
                return ClearType::Clear;
            if (value == U"fullcombo")
                return ClearType::FullCombo;
            if (value == U"perfect")
                return ClearType::Perfect;
            throw s3d::Error{ U"Invalid API response field: clearType" };
        }

        s3d::String SerializeClearType(ClearType value) {
            switch (value) {
                case ClearType::Failed:
                    return U"failed";
                case ClearType::Clear:
                    return U"clear";
                case ClearType::FullCombo:
                    return U"fullcombo";
                case ClearType::Perfect:
                    return U"perfect";
            }
            throw s3d::Error{ U"Invalid clear type." };
        }
    }

    User ParseUser(const s3d::JSON& json) {
        return {
            .id = Read<s3d::String>(json, U"id"),
            .card = Read<s3d::String>(json, U"card"),
            .display_name = Read<s3d::String>(json, U"displayName"),
            .rating = Read<s3d::uint32>(json, U"rating"),
            .xp = Read<s3d::uint32>(json, U"xp"),
            .credits = Read<s3d::uint32>(json, U"credits"),
            .is_public = Read<bool>(json, U"isPublic"),
            .is_admin = Read<bool>(json, U"isAdmin"),
            .created_at = Read<s3d::String>(json, U"createdAt"),
        };
    }

    s3d::Array<UserRecord> ParseRecords(const s3d::JSON& json) {
        return ReadArray<UserRecord>(json, [](const s3d::JSON& object) -> UserRecord {
            // The server response omits userId; the request already identifies the user.
            return {
                .id = Read<s3d::String>(object, U"id"),
                .sheet_id = Read<s3d::String>(object, U"sheetId"),
                .score = Read<s3d::uint32>(object, U"score"),
                .clear_type = ParseClearType(Read<s3d::String>(object, U"clearType")),
                .play_count = Read<s3d::uint32>(object, U"playCount"),
                .updated_at = Read<s3d::String>(object, U"updatedAt"),
            };
        });
    }

    PlayOptions ParsePlayOptions(const s3d::JSON& json) {
        return {
            .note_speed = Read<double>(json, U"noteSpeed"),
            .judgment_offset_ms = Read<s3d::int32>(json, U"judgmentOffset"),
        };
    }

    s3d::uint32 ParseCredits(const s3d::JSON& json) {
        return Read<s3d::uint32>(json, U"credits");
    }

    s3d::Array<Music> ParseCatalog(const s3d::JSON& json) {
        return ReadArray<Music>(json, [](const s3d::JSON& entry) -> Music {
            if (!entry.isObject() || !entry.hasElement(U"music") || !entry.hasElement(U"sheets")) {
                throw s3d::Error{ U"Expected music and sheets in the catalog." };
            }
            const auto music = entry[U"music"];
            return {
                .id = Read<s3d::String>(music, U"id"),
                .title = Read<s3d::String>(music, U"title"),
                .artist = Read<s3d::String>(music, U"artist"),
                .bpm = Read<double>(music, U"bpm"),
                .genre = Read<s3d::String>(music, U"genre"),
                .jacket = ReadAsset(music, U"jacket"),
                .audio = ReadAsset(music, U"audio"),
                .registration_date = Read<s3d::String>(music, U"registrationDate"),
                .is_test = Read<bool>(music, U"isTest"),
                .sheets = ReadArray<Sheet>(
                    entry[U"sheets"],
                    [](const s3d::JSON& sheet) -> Sheet {
                        const auto difficulty = Read<s3d::String>(sheet, U"difficulty");
                        if (difficulty != U"basic" && difficulty != U"advanced" && difficulty != U"master") {
                            throw s3d::Error{ U"Invalid API response field: difficulty" };
                        }
                        return {
                            .id = Read<s3d::String>(sheet, U"id"),
                            .music_id = Read<s3d::String>(sheet, U"musicId"),
                            .difficulty = difficulty,
                            .level = Read<double>(sheet, U"level"),
                            .notes_designer = Read<s3d::String>(sheet, U"notesDesigner"),
                            .chart = ReadAsset(sheet, U"chart"),
                        };
                    }
                ),
            };
        });
    }

    s3d::JSON SerializeRecords(s3d::StringView user_id, const s3d::Array<RecordSubmission>& records) {
        s3d::JSON json = s3d::JSON::Parse(U"[]");
        for (const auto& record : records) {
            s3d::JSON object;
            object[U"userId"] = s3d::String{ user_id };
            object[U"sheetId"] = record.sheet_id;
            object[U"score"] = record.score;
            object[U"clearType"] = SerializeClearType(record.clear_type);
            json.push_back(object);
        }
        return json;
    }

    s3d::JSON SerializePlayOptions(const PlayOptions& options) {
        if (!std::isfinite(options.note_speed)) {
            throw s3d::Error{ U"noteSpeed must be finite." };
        }
        s3d::JSON json;
        json[U"noteSpeed"] = options.note_speed;
        json[U"judgmentOffset"] = options.judgment_offset_ms;
        return json;
    }
}
