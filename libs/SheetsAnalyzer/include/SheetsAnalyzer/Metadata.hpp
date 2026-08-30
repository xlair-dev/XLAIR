#pragma once
#define NO_S3D_USING

#include <Siv3D.hpp>

namespace xlair::sheets {
    struct Difficulty {
        s3d::uint32 index = 0;
        double level = 0.0;
        s3d::String id;
        s3d::FilePath chart;
        s3d::String designer = U"Anonymous";
    };

    struct Metadata {
        s3d::FilePath source_path;
        s3d::String id;
        s3d::String title = U"Untitled";
        s3d::String title_sort = U"Untitled";
        s3d::String artist = U"Unknown Artist";
        s3d::String genre = U"Unspecified";
        s3d::FilePath music;
        s3d::FilePath jacket;
        s3d::URL url;
        double music_offset_seconds = 0.0;
        double demo_start_seconds = 0.0;
        double bpm = 120.0;
        s3d::Array<Difficulty> difficulties;
    };
}
