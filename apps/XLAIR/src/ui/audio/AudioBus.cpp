#include "AudioBus.hpp"

namespace xlair::ui::audio {
    void InitializeBuses() {
        // Siv3D reports 0 for buses that have not been created yet.
        for (const auto bus : { Music, UISound, GameplaySound, Voice }) {
            GlobalAudio::BusSetVolume(bus, bus == GameplaySound ? 0.55 : 1.0);
        }
    }
}
