#pragma once

#include <Siv3D.hpp>

namespace xlair::api {
    enum class ClearType {
        Failed,
        Clear,
        FullCombo,
        Perfect,
    };

    struct User {
        s3d::String id;
        s3d::String card;
        s3d::String display_name;
        s3d::uint32 rating = 0;
        s3d::uint32 xp = 0;
        s3d::uint32 credits = 0;
        bool is_public = false;
        bool is_admin = false;
        s3d::String created_at;
    };

    struct PlayOptions {
        double note_speed = 1.0;
        s3d::int32 judgment_offset_ms = 0;
    };

    struct RecordSubmission {
        s3d::String sheet_id;
        s3d::uint32 score = 0;
        ClearType clear_type = ClearType::Failed;
    };

    struct UserRecord {
        s3d::String id;
        s3d::String sheet_id;
        s3d::uint32 score = 0;
        ClearType clear_type = ClearType::Failed;
        s3d::uint32 play_count = 0;
        s3d::String updated_at;
    };

    struct Asset {
        // Preserve server URL references, including origin-relative paths.
        s3d::URL url;
        s3d::String updated_at;
    };

    struct Sheet {
        s3d::String id;
        s3d::String music_id;
        s3d::String difficulty;
        double level = 0;
        s3d::String notes_designer;
        s3d::Optional<Asset> chart;
    };

    struct Music {
        s3d::String id;
        s3d::String title;
        s3d::String artist;
        double bpm = 0;
        s3d::String genre;
        s3d::Optional<Asset> jacket;
        s3d::Optional<Asset> audio;
        s3d::String registration_date;
        bool is_test = false;
        s3d::Array<Sheet> sheets;
    };
}
