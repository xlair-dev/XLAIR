#pragma once

#include <Siv3D/StringView.hpp>

#define XLAIR_VERSION_MAJOR 0
#define XLAIR_VERSION_MINOR 1
#define XLAIR_VERSION_PATCH 0
#define XLAIR_VERSION_PRERELEASE_TAG U"alpha"

#define XLAIR_DETAIL_STRINGIFY_IMPL(value) #value
#define XLAIR_DETAIL_STRINGIFY(value) XLAIR_DETAIL_STRINGIFY_IMPL(value)
#define XLAIR_DETAIL_UTF32_IMPL(value) U##value
#define XLAIR_DETAIL_UTF32(value) XLAIR_DETAIL_UTF32_IMPL(value)
#define XLAIR_DETAIL_UTF32_STRINGIFY(value) XLAIR_DETAIL_UTF32(XLAIR_DETAIL_STRINGIFY(value))

#define XLAIR_VERSION_SHORT_STRING                                                                                     \
    XLAIR_DETAIL_UTF32_STRINGIFY(XLAIR_VERSION_MAJOR)                                                                  \
    U"." XLAIR_DETAIL_UTF32_STRINGIFY(XLAIR_VERSION_MINOR) U"." XLAIR_DETAIL_UTF32_STRINGIFY(XLAIR_VERSION_PATCH)
#define XLAIR_VERSION_STRING XLAIR_VERSION_SHORT_STRING U" " XLAIR_VERSION_PRERELEASE_TAG

namespace xlair::app::version {
    inline constexpr int Major = XLAIR_VERSION_MAJOR;
    inline constexpr int Minor = XLAIR_VERSION_MINOR;
    inline constexpr int Patch = XLAIR_VERSION_PATCH;

    inline constexpr int Code = (Major * 100 * 100) + (Minor * 100) + Patch;

    inline constexpr s3d::StringView PrereleaseTag{ XLAIR_VERSION_PRERELEASE_TAG };
    inline constexpr s3d::StringView ShortString{ XLAIR_VERSION_SHORT_STRING };
    inline constexpr s3d::StringView String{ XLAIR_VERSION_STRING };
}
