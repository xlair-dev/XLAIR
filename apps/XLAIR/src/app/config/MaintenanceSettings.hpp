#pragma once

#include "Config.hpp"

namespace xlair::app {
    // Only settings editable in Maintenance are persisted by its save command.
    struct MaintenanceSettings {
        explicit MaintenanceSettings(const Config& config)
            : autoplay{ config.system.autoplay }, music_volume{ config.audio.music_volume },
              ui_sound_volume{ config.audio.ui_sound_volume },
              gameplay_sound_volume{ config.audio.gameplay_sound_volume },
              highlight_touch_threshold{ config.controller.highlight_touch_threshold },
              ground_slider_touch_threshold{ config.controller.ground_slider.touch_threshold },
              v1_touch_threshold{ config.controller.v1.touch_threshold },
              latency_offset_seconds{ config.input.latency_offset_seconds },
              api_timeout_seconds{ config.api.timeout_seconds } {}

        bool operator==(const MaintenanceSettings&) const = default;

        bool autoplay;
        double music_volume;
        double ui_sound_volume;
        double gameplay_sound_volume;
        bool highlight_touch_threshold;
        uint8 ground_slider_touch_threshold;
        uint8 v1_touch_threshold;
        double latency_offset_seconds;
        double api_timeout_seconds;
    };
}
