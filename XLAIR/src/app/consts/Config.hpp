#pragma once
#include <Common.hpp>
#include "app/types/Config.hpp"

namespace app::consts {
    namespace defaults {
        static inline constexpr int32 Width = 1920;
        static inline constexpr int32 Height = 1080;
        static inline constexpr bool Sizable = true;
        static inline constexpr bool Fullscreen = false;
        static inline constexpr ColorF LetterboxColor{ 0 };

        static inline constexpr bool Arcade = false;
    }
}

