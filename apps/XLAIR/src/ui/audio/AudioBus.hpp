#pragma once

#include "Common.hpp"

namespace xlair::ui::audio {
    inline constexpr MixBus Music = MixBus0;
    inline constexpr MixBus UISound = MixBus1;
    inline constexpr MixBus GameplaySound = MixBus2;
    inline constexpr MixBus Voice = MixBus3;
}
