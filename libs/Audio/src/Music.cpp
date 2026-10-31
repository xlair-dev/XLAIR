#include <Audio.hpp>

#include <utility>

namespace xlair::audio {
    s3d::Audio LoadMusic(const s3d::FilePathView path) {
        s3d::Wave wave{ path };
        if (wave.isEmpty()) {
            return {};
        }

        if (wave.sampleRate() != PlaybackSampleRate) {
            s3d::Logger << s3d::Format(
                U"[Audio] Resampling music: ",
                path,
                U" (",
                wave.sampleRate(),
                U" Hz -> ",
                PlaybackSampleRate,
                U" Hz)."
            );
        }
        return s3d::Audio{ ResampleForPlayback(std::move(wave)) };
    }
}
