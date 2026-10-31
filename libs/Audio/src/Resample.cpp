#include <Audio.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <numeric>
#include <stdexcept>

namespace xlair::audio {
    namespace {
        constexpr std::size_t FilterRadius = 32;
        constexpr double CutoffMargin = 0.94;

        [[nodiscard]]
        double FilterWeight(const double distance, const double cutoff, const std::size_t radius) {
            const double window_position = distance / radius;
            if (std::abs(window_position) >= 1.0) {
                return 0.0;
            }

            const double angle = std::numbers::pi * cutoff * distance;
            const double sinc = std::abs(angle) < 1e-12 ? 1.0 : std::sin(angle) / angle;
            const double window = 0.42 + 0.5 * std::cos(std::numbers::pi * window_position) +
                                  0.08 * std::cos(2.0 * std::numbers::pi * window_position);
            return cutoff * sinc * window;
        }

        [[nodiscard]]
        std::size_t OutputSize(const std::size_t input_size, const s3d::uint32 input_rate) {
            constexpr auto Maximum = std::numeric_limits<s3d::uint64>::max();
            if (input_size > (Maximum - input_rate / 2) / PlaybackSampleRate) {
                throw std::length_error{ "Music is too long to resample." };
            }

            const auto size = std::max<s3d::uint64>(
                1,
                (static_cast<s3d::uint64>(input_size) * PlaybackSampleRate + input_rate / 2) / input_rate
            );
            if (size > std::numeric_limits<std::size_t>::max()) {
                throw std::length_error{ "Resampled music exceeds the supported size." };
            }
            return static_cast<std::size_t>(size);
        }
    }

    s3d::Wave ResampleForPlayback(s3d::Wave wave) {
        if (wave.isEmpty()) {
            return s3d::Wave{ s3d::Arg::sampleRate = PlaybackSampleRate };
        }
        const auto input_rate = wave.sampleRate();
        if (input_rate == PlaybackSampleRate) {
            return wave;
        }
        if (input_rate < s3d::Wave::MinSampleRate || input_rate > s3d::Wave::MaxSampleRate ||
            wave.size() > static_cast<std::size_t>(std::numeric_limits<s3d::int64>::max())) {
            throw std::invalid_argument{ "Invalid music sample rate or size." };
        }

        const auto divisor = std::gcd(input_rate, PlaybackSampleRate);
        const s3d::uint64 source_step = input_rate / divisor;
        const std::size_t phase_count = PlaybackSampleRate / divisor;
        const double scale = std::min(1.0, static_cast<double>(PlaybackSampleRate) / input_rate);
        const auto radius = static_cast<std::size_t>(std::ceil(FilterRadius / scale));
        const std::size_t taps = 2 * radius;
        const double cutoff = scale * CutoffMargin;

        // Precompute a Blackman-windowed sinc for each exact rational phase.
        // Widen the filter when downsampling so frequencies above the new
        // Nyquist limit are suppressed instead of folding into the music.
        s3d::Array<double> filters(phase_count * taps);
        for (std::size_t phase = 0; phase < phase_count; ++phase) {
            const double fraction = static_cast<double>(phase) / phase_count;
            auto* weights = filters.data() + phase * taps;
            double sum = 0.0;
            for (std::size_t tap = 0; tap < taps; ++tap) {
                const double distance = static_cast<double>(tap) - radius + 1.0 - fraction;
                weights[tap] = FilterWeight(distance, cutoff, radius);
                sum += weights[tap];
            }
            for (std::size_t tap = 0; tap < taps; ++tap) {
                weights[tap] /= sum;
            }
        }

        s3d::Wave output{ OutputSize(wave.size(), input_rate), s3d::Arg::sampleRate = PlaybackSampleRate };
        std::size_t source_index = 0;
        s3d::uint64 phase = 0;
        const auto last_source = static_cast<s3d::int64>(wave.size() - 1);
        for (auto& sample : output) {
            const auto* weights = filters.data() + phase * taps;
            const auto first_source = static_cast<s3d::int64>(source_index) - static_cast<s3d::int64>(radius) + 1;
            double left = 0.0;
            double right = 0.0;
            if (first_source >= 0 &&
                first_source <= static_cast<s3d::int64>(wave.size()) - static_cast<s3d::int64>(taps)) {
                const auto* source = wave.data() + first_source;
                for (std::size_t tap = 0; tap < taps; ++tap) {
                    left += weights[tap] * source[tap].left;
                    right += weights[tap] * source[tap].right;
                }
            } else {
                // Extend endpoint samples, preserving DC and supporting even
                // a one-frame clip without introducing a time offset.
                for (std::size_t tap = 0; tap < taps; ++tap) {
                    const auto index =
                        std::clamp<s3d::int64>(first_source + static_cast<s3d::int64>(tap), 0, last_source);
                    left += weights[tap] * wave[index].left;
                    right += weights[tap] * wave[index].right;
                }
            }
            sample = s3d::WaveSample{ static_cast<float>(left), static_cast<float>(right) };

            // Advance an integer fraction, never a rounded interval or a
            // floating accumulator. The filter is centered at this position,
            // so no FIR group delay is added to chart/music alignment.
            phase += source_step;
            source_index += phase / phase_count;
            phase %= phase_count;
        }
        return output;
    }
}
