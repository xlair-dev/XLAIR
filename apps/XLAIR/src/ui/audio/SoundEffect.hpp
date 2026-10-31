#pragma once

#include "Common.hpp"

namespace xlair::ui::audio {
    enum class SoundEffect {
        Navigate,
        ChangeDifficulty,
        Confirm,
        TextInput,
        Next,
        Back,
        Finish,
        CoinInsert,
        LoginComplete,
        PopupError,
        TimerNormal,
    };

    void PlaySoundEffect(SoundEffect sound);
}
