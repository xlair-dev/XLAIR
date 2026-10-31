#pragma once

#include "Common.hpp"

namespace xlair::ui::audio {
    enum class SoundEffect {
        Navigate,
        ChangeDifficulty,
        Confirm,
        Finish,
        CoinInsert,
        LoginComplete,
        PopupError,
        TimerNormal,
    };

    void PlaySoundEffect(SoundEffect sound);
}
