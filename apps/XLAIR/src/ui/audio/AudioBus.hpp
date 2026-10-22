#pragma once

#include "Common.hpp"

namespace xlair::ui::audio {
    inline constexpr MixBus Music = MixBus0;
    inline constexpr MixBus UISound = MixBus1;
    // All judgement one-shots and sustained hold loops share this bus.
    inline constexpr MixBus GameplaySound = MixBus2;
    inline constexpr MixBus Voice = MixBus3;

    // Call once at startup, before playback or volume controls access the buses.
    void InitializeBuses();
}
