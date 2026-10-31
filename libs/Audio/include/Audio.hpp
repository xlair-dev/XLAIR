#pragma once

#include <Siv3D.hpp>

namespace xlair::audio {
    // Workaround for SoLoud's rate-conversion drift with the stock 44.1 kHz
    // mixer/buses. This does not change or force the backend's sample rate.
    inline constexpr s3d::uint32 PlaybackSampleRate = 44'100;

    /// Band-limit and resample PCM without changing its time origin.
    /// Already-normalized PCM is returned unchanged; the input can be moved in.
    [[nodiscard]]
    s3d::Wave ResampleForPlayback(s3d::Wave wave);

    /// Decode and normalize music in memory. Call from a loading task, not the UI thread.
    /// The original file is never modified. A decode failure returns an empty Audio.
    [[nodiscard]]
    s3d::Audio LoadMusic(s3d::FilePathView path);
}
