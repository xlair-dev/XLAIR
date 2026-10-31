#include <ThirdParty/Catch2/catch.hpp>

#include <Audio.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <utility>

namespace audio = xlair::audio;

namespace {
    s3d::Wave Tone(const s3d::uint32 rate, const double frequency) {
        s3d::Wave wave{ rate / 5, s3d::Arg::sampleRate = rate };
        for (std::size_t index = 0; index < wave.size(); ++index) {
            const auto value = static_cast<float>(std::sin(2.0 * std::numbers::pi * frequency * index / rate));
            wave[index] = s3d::WaveSample{ value, -0.5f * value };
        }
        return wave;
    }

    double InteriorRMS(const s3d::Wave& wave) {
        double energy = 0.0;
        for (std::size_t index = 200; index < wave.size() - 200; ++index) {
            energy += static_cast<double>(wave[index].left) * wave[index].left;
        }
        return std::sqrt(energy / (wave.size() - 400));
    }
}

TEST_CASE("Music at the playback rate is passed through without copying", "[Audio][Resample]") {
    auto wave = Tone(audio::PlaybackSampleRate, 1'000.0);
    const auto* data = wave.data();
    const auto size = wave.size();
    const auto sample = wave[17];
    const auto normalized = audio::ResampleForPlayback(std::move(wave));
    CHECK(normalized.data() == data);
    CHECK(normalized.size() == size);
    CHECK(normalized.sampleRate() == audio::PlaybackSampleRate);
    CHECK(normalized[17].left == sample.left);
    CHECK(normalized[17].right == sample.right);
}

TEST_CASE("Empty music remains empty", "[Audio][Resample]") {
    const auto normalized = audio::ResampleForPlayback(s3d::Wave{ s3d::Arg::sampleRate = 48'000 });
    CHECK(normalized.isEmpty());
    CHECK(normalized.sampleRate() == audio::PlaybackSampleRate);
}

TEST_CASE("Resampling preserves duration and independent stereo channels", "[Audio][Resample]") {
    for (const s3d::uint32 rate : { 8'000, 22'050, 32'000, 48'000, 88'200, 96'000, 192'000 }) {
        CAPTURE(rate);
        s3d::Wave wave{ rate / 4 + 1, s3d::WaveSample{ 0.25f, -0.5f }, s3d::Arg::sampleRate = rate };
        const auto expected = static_cast<std::size_t>(std::llround(wave.size() * 44'100.0 / rate));
        const double duration = wave.lengthSec();
        const auto normalized = audio::ResampleForPlayback(std::move(wave));
        REQUIRE(normalized.size() == expected);
        CHECK(normalized.sampleRate() == audio::PlaybackSampleRate);
        CHECK(std::abs(normalized.lengthSec() - duration) <= (0.5 + 1e-9) / audio::PlaybackSampleRate);
        CHECK(normalized.all([](const s3d::WaveSample sample) {
            return std::abs(sample.left - 0.25f) < 1e-6f && std::abs(sample.right + 0.5f) < 1e-6f;
        }));
    }
}

TEST_CASE("A single-frame clip can be upsampled or downsampled safely", "[Audio][Resample]") {
    for (const s3d::uint32 rate : { 4'000, 48'000, 192'000 }) {
        CAPTURE(rate);
        const auto normalized = audio::ResampleForPlayback(
            s3d::Wave{ std::size_t{ 1 }, s3d::WaveSample{ 0.25f, -0.5f }, s3d::Arg::sampleRate = rate }
        );
        REQUIRE_FALSE(normalized.isEmpty());
        CHECK(normalized.front().left == Approx(0.25f).margin(1e-6));
        CHECK(normalized.back().right == Approx(-0.5f).margin(1e-6));
    }
}

TEST_CASE("Unsupported PCM rates are rejected before allocating filters", "[Audio][Resample]") {
    for (const s3d::uint32 rate : { s3d::uint32{ 0 }, s3d::uint32{ 1 }, std::numeric_limits<s3d::uint32>::max() }) {
        CAPTURE(rate);
        CHECK_THROWS_AS(
            audio::ResampleForPlayback(s3d::Wave{ std::size_t{ 1 }, s3d::Arg::sampleRate = rate }),
            std::invalid_argument
        );
    }
}

TEST_CASE("Resampling retains audible frequencies without changing their phase", "[Audio][Resample]") {
    for (const s3d::uint32 rate : { 22'050, 48'000, 96'000, 192'000 }) {
        CAPTURE(rate);
        const auto normalized = audio::ResampleForPlayback(Tone(rate, 1'000.0));
        double maximum_error = 0.0;
        for (std::size_t index = 200; index < normalized.size() - 200; ++index) {
            const double expected = std::sin(2.0 * std::numbers::pi * 1'000.0 * index / audio::PlaybackSampleRate);
            maximum_error = std::max(maximum_error, std::abs(normalized[index].left - expected));
            maximum_error = std::max(maximum_error, std::abs(normalized[index].right + 0.5 * expected));
        }
        CHECK(maximum_error < 0.0002);
    }
}

TEST_CASE("Downsampling suppresses frequencies above the playback Nyquist limit", "[Audio][Resample]") {
    const auto audible = audio::ResampleForPlayback(Tone(48'000, 1'000.0));
    const auto ultrasonic = audio::ResampleForPlayback(Tone(48'000, 23'000.0));
    CHECK(InteriorRMS(audible) > 0.70);
    CHECK(InteriorRMS(ultrasonic) < 0.001);
}

TEST_CASE("Pulse positions stay aligned throughout a three-minute song", "[Audio][Resample][Timing]") {
    constexpr s3d::uint32 SourceRate = 48'000;
    s3d::Wave wave{ std::size_t{ SourceRate } * 180, s3d::WaveSample{ 0.0f }, s3d::Arg::sampleRate = SourceRate };
    for (const int second : { 10, 90, 170 }) {
        const auto center = static_cast<std::size_t>(second) * SourceRate;
        for (int offset = -100; offset <= 100; ++offset) {
            const auto value = static_cast<float>(std::exp(-static_cast<double>(offset * offset) / 400.0));
            wave[static_cast<std::size_t>(static_cast<s3d::int64>(center) + offset)] = s3d::WaveSample{ value, value };
        }
    }

    const auto normalized = audio::ResampleForPlayback(std::move(wave));
    REQUIRE(normalized.size() == std::size_t{ audio::PlaybackSampleRate } * 180);
    for (const int second : { 10, 90, 170 }) {
        CAPTURE(second);
        const auto center = static_cast<std::size_t>(second) * audio::PlaybackSampleRate;
        const auto peak = std::max_element(
            normalized.begin() + center - 100,
            normalized.begin() + center + 100,
            [](const auto& left, const auto& right) {
                return left.left < right.left;
            }
        );
        const auto sample = static_cast<s3d::int64>(peak - normalized.begin());
        CHECK(std::abs(sample - static_cast<s3d::int64>(center)) <= 1);
        CHECK(peak->left > 0.99f);
    }
}
