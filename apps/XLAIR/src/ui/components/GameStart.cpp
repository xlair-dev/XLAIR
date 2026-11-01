#include "GameStart.hpp"

#include "ui/Design.hpp"
#include "ui/assets/Assets.hpp"
#include "ui/presentation/DifficultyText.hpp"
#include "ui/theme/DifficultyTheme.hpp"

namespace xlair::ui::components {
    namespace {
        constexpr Vec2 JacketCenter = GameStartJacketBounds.center();
        constexpr double JacketSize = GameStartJacketBounds.size.x;
        constexpr ColorF Background{ U"#202226" };
        constexpr ColorF Chevron{ U"#181A1E" };

        void DrawBackgroundHalf(const bool right, const double offset, const double enter) {
            const Transformer2D transform{ right ? Mat3x2::Scale(-1.0, 1.0) *
                                                       Mat3x2::Translate(DesignSize.x + offset, 0.0)
                                                 : Mat3x2::Translate(-offset, 0.0) };
            RectF{ 0, 0, DesignSize.x / 2.0, DesignSize.y }.draw(Background);

            const double drift = 24.0 * (1.0 - enter);
            Quad{ Vec2{ 0, 0 }, Vec2{ 350 - drift, 0 }, Vec2{ 890 - drift, 540 }, Vec2{ 540 - drift, 540 } }.draw(
                Chevron
            );
            Quad{ Vec2{ 540 - drift, 540 }, Vec2{ 890 - drift, 540 }, Vec2{ 350 - drift, 1080 }, Vec2{ 0, 1080 } }.draw(
                Chevron
            );
            Quad{ Vec2{ 0, 170 }, Vec2{ 0, 330 }, Vec2{ 210 - drift, 540 }, Vec2{ 370 - drift, 540 } }.draw(Chevron);
            Quad{ Vec2{ 210 - drift, 540 }, Vec2{ 370 - drift, 540 }, Vec2{ 0, 910 }, Vec2{ 0, 750 } }.draw(Chevron);
        }

        void DrawWord(const StringView word, const Vec2& center, const double opacity) {
            const auto text = FontAsset{ assets::font::Display }(word);
            constexpr double FontSize = 130.0;
            const auto outline = TextStyle::Outline(0.025, 0.025, ColorF{ 1.0, 0.75 * opacity });
            text.drawAt(outline, FontSize, center, ColorF{ 1.0, 0.0 });

            const Transformer2D reflection{ Mat3x2::Scale(1.0, -0.8, center) * Mat3x2::Translate(0.0, 125.0) };
            text.drawAt(
                TextStyle::Outline(0.025, 0.025, ColorF{ 1.0, 0.10 * opacity }),
                FontSize,
                center,
                ColorF{ 1.0, 0.0 }
            );
        }

        void DrawFittedText(
            const StringView value,
            const double font_size,
            const double maximum_width,
            const Vec2& center,
            const ColorF& color
        ) {
            const auto text = FontAsset{ assets::font::Text }(value);
            const double width = text.region(font_size).w;
            text.drawAt(font_size * Min(1.0, maximum_width / Max(1.0, width)), center, color);
        }

        void DrawJacketFrame(const ColorF& color) {
            constexpr double HalfSize = JacketSize / 2.0 + 20.0;
            constexpr double CornerLength = 55.0;
            for (const double x : { -1.0, 1.0 }) {
                for (const double y : { -1.0, 1.0 }) {
                    const Vec2 corner = JacketCenter + Vec2{ x * HalfSize, y * HalfSize };
                    Line{ corner, corner.movedBy(-x * CornerLength, 0.0) }.draw(2.0, color);
                    Line{ corner, corner.movedBy(0.0, -y * CornerLength) }.draw(2.0, color);
                }
            }
        }
    }

    void DrawGameStartBackground(const double elapsed, const double reveal, const double opacity) {
        const double enter = EaseOutCubic(Clamp(elapsed / GameStartEnterDuration, 0.0, 1.0));
        const double leave = EaseInOutCubic(Clamp(reveal, 0.0, 1.0));
        const double offset = DesignSize.x / 2.0 * leave;
        const auto color_mul = Graphics2D::GetColorMul();
        const ScopedColorMul2D color{
            color_mul.x,
            color_mul.y,
            color_mul.z,
            color_mul.w * Clamp(opacity, 0.0, 1.0),
        };
        DrawBackgroundHalf(false, offset, enter);
        DrawBackgroundHalf(true, offset, enter);
    }

    void DrawGameStart(const GameStartData& data, const double elapsed, const double reveal, const bool draw_jacket) {
        const double enter = EaseOutCubic(Clamp(elapsed / GameStartEnterDuration, 0.0, 1.0));
        const double leave = EaseInOutCubic(Clamp(reveal, 0.0, 1.0));
        const double opacity = enter * (1.0 - leave);
        DrawGameStartBackground(elapsed, reveal);

        if (opacity <= 0.0) {
            return;
        }

        const double word_offset = (1.0 - enter) * 60.0 + leave * 180.0;
        DrawWord(U"GAME", Vec2{ 385.0 - word_offset, 500.0 }, opacity);
        DrawWord(U"START", Vec2{ 1535.0 + word_offset, 500.0 }, opacity);

        const auto colors = theme::GetDifficultyTheme(data.difficulty_index);
        const Transformer2D content_transform{ Mat3x2::Scale(0.98 + 0.02 * enter, JacketCenter) *
                                               Mat3x2::Translate(0.0, 12.0 * (1.0 - enter)) };
        if (draw_jacket) {
            GameStartJacketBounds.draw(ColorF{ 0.05, opacity });
            if (data.jacket) {
                data.jacket.fitted(JacketSize, JacketSize).drawAt(JacketCenter, ColorF{ 1.0, opacity });
            }
        }
        DrawJacketFrame(colors.accent.withA(opacity));

        {
            const Vec2 center = JacketCenter.movedBy(-245.0, 0.0);
            const Transformer2D rotation{ Mat3x2::Rotate(-90_deg, center) };
            const auto difficulty =
                FontAsset{ assets::font::Label }(presentation::DifficultyLabel(data.difficulty_index).uppercased());
            const double font_size = 40.0 * Min(1.0, JacketSize / Max(1.0, difficulty.region(40.0).w));
            difficulty.drawAt(font_size, center, colors.accent.withA(opacity));
        }

        DrawFittedText(data.title, 64.0, 1200.0, Vec2{ JacketCenter.x, 715.0 }, ColorF{ 1.0, opacity });
        Line{ JacketCenter.x - 220.0, 766.0, JacketCenter.x + 220.0, 766.0 }.draw(1.0, ColorF{ 1.0, 0.7 * opacity });
        DrawFittedText(
            U"作曲：{}"_fmt(data.artist),
            30.0,
            1100.0,
            Vec2{ JacketCenter.x, 810.0 },
            ColorF{ 1.0, 0.9 * opacity }
        );
        DrawFittedText(
            U"譜面制作：{}"_fmt(data.designer),
            30.0,
            1100.0,
            Vec2{ JacketCenter.x, 850.0 },
            ColorF{ 1.0, 0.9 * opacity }
        );
    }
}
