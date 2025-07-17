#pragma once
#define NO_S3D_USING
#include <Siv3D.hpp>

namespace SheetsAnalyzer {
    namespace Constant {
        constexpr s3d::StringView DefaultTitle = U"Untitled";
        constexpr s3d::StringView DefaultTitleSort = U"Untitled";
        constexpr s3d::StringView DefaultArtist = U"Unknown Artist";
        constexpr s3d::StringView DefaultDesiner = U"Anonymous";
        constexpr s3d::StringView DefaltGenre = U"Unspecified";
        constexpr double DefaultMusicOffset = 0.0;
        constexpr double DefaultBPM = 120.0;

        constexpr size_t MaxDifficulties = 10;
    }
}
