#include "AudioBus.hpp"

namespace xlair::ui::audio {
    void InitializeBuses() {
        // Siv3D reports 0 for buses that have not been created yet.
        ApplyVolumes(app::Config::Audio{});
        GlobalAudio::BusSetVolume(Voice, 1.0);
    }

    void ApplyVolumes(const app::Config::Audio& volumes) {
        GlobalAudio::BusSetVolume(Music, volumes.music_volume);
        GlobalAudio::BusSetVolume(UISound, volumes.ui_sound_volume);
        GlobalAudio::BusSetVolume(GameplaySound, volumes.gameplay_sound_volume);
    }
}
