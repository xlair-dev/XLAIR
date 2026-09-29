#pragma once

#include "Common.hpp"

namespace xlair::ui::audio {
    enum class SoundEffect {
        Navigate,
        ChangeDifficulty,
        Confirm,
    };

    void PlaySoundEffect(SoundEffect sound);
}
