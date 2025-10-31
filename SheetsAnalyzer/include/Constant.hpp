#pragma once
#define NO_S3D_USING
#include <Siv3D.hpp>

namespace SheetsAnalyzer {
    namespace Constant {
        constexpr inline s3d::int32 Version = 1;

        const inline s3d::String DefaultTitle = U"Untitled";
        const inline s3d::String DefaultTitleSort = U"Untitled";
        const inline s3d::String DefaultArtist = U"Unknown Artist";
        const inline s3d::String DefaltGenre = U"Unspecified";
        const inline s3d::String DefaultDesigner = U"Anonymous";
        constexpr inline double DefaultMusicOffset = 0.0;
        constexpr inline double DefaultDemoStart = 0.0;
        constexpr inline double DefaultBPM = 120.0;

        constexpr inline size_t MaxDifficulties = 10;
    }
}
