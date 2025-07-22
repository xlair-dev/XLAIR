#pragma once
#define NO_S3D_USING
#include <Siv3D.hpp>
#include "Constant.hpp"

namespace SheetsAnalyzer {
    struct Difficulty {
        double level = 0.0;
        s3d::FilePath src;
        s3d::String designer;
    };

    struct Metadata {
        s3d::FilePath path;
        s3d::String id;
        s3d::String title;
        s3d::String title_sort;
        s3d::String artist;
        s3d::String genre;
        s3d::String music;
        s3d::String jacket;
        s3d::URL url;
        double music_offset;
        double bpm;

        std::array<Difficulty, Constant::MaxDifficulties> difficulties;
    };
}
