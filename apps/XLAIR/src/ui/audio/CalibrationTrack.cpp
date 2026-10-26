#include "CalibrationTrack.hpp"

#include "app/flows/LatencyCalibration.hpp"

#include <cmath>
#include <utility>

namespace xlair::ui::audio {
    Audio CreateCalibrationTrack() {
        using Calibration = app::flows::LatencyCalibration;
        constexpr uint32 SampleRate = 48000;
        const auto first = static_cast<std::size_t>(Calibration::FirstBeatMs * SampleRate / 1000.0);
        const auto interval = static_cast<std::size_t>(Calibration::BeatIntervalMs * SampleRate / 1000.0);
        Wave wave{ first + interval, WaveSample{ 0.0f }, Arg::sampleRate = SampleRate };
        // Bake the click into the waveform so frame scheduling and asset silence
        // cannot move its onset relative to the playback sample clock.
        for (std::size_t index = 0; index < SampleRate / 50; ++index) {
            const double time = static_cast<double>(index) / SampleRate;
            wave[first + index] =
                static_cast<float>(0.4 * std::sin(Math::TwoPi * 1500.0 * time) * std::exp(-time / 0.004));
        }
        return Audio{ std::move(wave),
                      Arg::loopBegin = static_cast<uint64>(first),
                      Arg::loopEnd = static_cast<uint64>(first + interval) };
    }
}
