#pragma once
#define NO_S3D_USING
#include <Siv3D.hpp>

namespace SheetsAnalyzer {
    namespace Constant {
        constexpr s3d::int32 Version = 1;

        const s3d::String DefaultTitle = U"Untitled";
        const s3d::String DefaultTitleSort = U"Untitled";
        const s3d::String DefaultArtist = U"Unknown Artist";
        const s3d::String DefaultDesiner = U"Anonymous";
        const s3d::String DefaltGenre = U"Unspecified";
        constexpr double DefaultMusicOffset = 0.0;
        constexpr double DefaultBPM = 120.0;

        constexpr size_t MaxDifficulties = 10;
    }
}
