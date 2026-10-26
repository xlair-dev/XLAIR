#include "LatencyCalibrationView.hpp"

#include "ui/assets/Assets.hpp"

#include <cmath>

namespace xlair::ui::components {
    void DrawLatencyCalibration(const LatencyCalibrationView& view, const Size& viewport) {
        using Calibration = app::flows::LatencyCalibration;
        const FontAsset font{ assets::font::Text };
        const auto mean = view.calibration.averageMs();
        const String average = mean ? U"平均差分: {:+.1f} ms"_fmt(*mean) : U"平均差分: 計測待ち";
        font(average).draw(32, Vec2{ 48, 128 }, Palette::White);
        font(U"計測回数: {}"_fmt(view.calibration.tapCount())).draw(26, Vec2{ 48, 184 }, Palette::White);
        if (view.applied_offset_ms) {
            font(U"適用中の補正: {:+.1f} ms"_fmt(*view.applied_offset_ms)).draw(26, Vec2{ 48, 232 }, Palette::White);
            if (mean) {
                font(U"補正後の平均差分: {:+.1f} ms"_fmt(*mean - *view.applied_offset_ms))
                    .draw(24, Vec2{ 48, 278 }, Palette::White);
            }
        }
        const StringView status = !view.audio_playing        ? U"音声再生エラー"
                                  : view.applied             ? U"適用済み"
                                  : view.calibration.ready() ? U"適用可"
                                                             : U"計測中…";
        font(status).draw(26, Vec2{ 48, 330 }, Palette::White);

        const double left = viewport.x * 0.43;
        const double right = viewport.x - 220.0;
        constexpr double top = 140.0;
        const double baseline = viewport.y * 0.70;
        const double bottom = viewport.y - 150.0;
        const double pixels_per_ms = (baseline - top) / 1000.0;
        Line{ left, top, left, bottom }.draw(1, Palette::Gray);
        Line{ right, top, right, bottom }.draw(1, Palette::Gray);
        const double phase = std::fmod(view.time_ms, Calibration::BeatIntervalMs);
        const bool flash = view.time_ms >= Calibration::FirstBeatMs && phase < 100.0;
        Line{ left, baseline, right, baseline }.draw(flash ? 10 : 6, Palette::Red);
        font(U"0 ms").draw(20, Vec2{ right + 12, baseline - 30 }, Palette::Red);
        const auto beat =
            static_cast<int64>(std::floor((view.time_ms - Calibration::FirstBeatMs) / Calibration::BeatIntervalMs));
        for (int64 index = Max(int64{ 0 }, beat - 1); index <= beat + 2; ++index) {
            const double time = Calibration::FirstBeatMs + index * Calibration::BeatIntervalMs;
            const double y = baseline + (view.time_ms - time) * pixels_per_ms;
            if (y >= top && y <= bottom) {
                Line{ left, y, right, y }.draw(3, Palette::White);
            }
        }
        for (const auto& tap : view.calibration.taps()) {
            const double opacity = Clamp(1.0 - (view.time_ms - tap.time_ms) / 4000.0, 0.0, 1.0);
            const double y = baseline + tap.offset_ms * pixels_per_ms;
            if (opacity <= 0.0 || y < top || y > bottom) {
                continue;
            }
            const ColorF color{ tap.accepted ? Palette::Cyan : Palette::Gray, opacity };
            Line{ left, y, right, y }.draw(1, color);
            font(U"{:+.0f} ms"_fmt(tap.offset_ms)).draw(18, Vec2{ right + 12, y }, color);
        }
    }
}
