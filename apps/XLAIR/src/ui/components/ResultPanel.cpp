#include "ResultPanel.hpp"

#include "ui/assets/Assets.hpp"
#include "ui/presentation/ResultRating.hpp"
#include "ui/presentation/ScoringText.hpp"
#include "ui/theme/Palette.hpp"

#include <array>

namespace xlair::ui::components {
    namespace {
        constexpr Size PanelSize{ 1400, 760 };

        [[nodiscard]]
        double Reveal(const double elapsed, const double start, const double duration) {
            return Easing::Cubic(Clamp((elapsed - start) / duration, 0.0, 1.0));
        }

        void DrawFrame(const double opacity) {
            Rect{ PanelSize }.draw(ColorF{ 1.0, 0.7 * opacity });
            constexpr std::array widths{ 16, 8, 4 };
            constexpr std::array offsets{ 0, 24, 40 };
            for (std::size_t index = 0; index < widths.size(); ++index) {
                Rect{ offsets[index], 0, widths[index], PanelSize.y }.draw(theme::Palette::Cyan.withA(opacity));
                Rect{ PanelSize.x - offsets[index] - widths[index], 0, widths[index], PanelSize.y }.draw(
                    theme::Palette::Cyan.withA(opacity)
                );
            }
        }

        void DrawScore(const uint32 score, const double opacity, const double progress) {
            const auto font = FontAsset{ assets::font::Text };
            const auto animated_score = static_cast<uint32>(score * progress);
            const String score_text = Format(animated_score);
            const String padding(7 - Min(std::size_t{ 7 }, score_text.size()), U'0');
            const auto leading = font(padding).draw(140, Vec2{ 130, 50 }, theme::Palette::Gray.withA(0.6 * opacity));
            font(score_text).draw(140, Vec2{ 130 + leading.w, 50 }, theme::Palette::Cyan.withA(opacity));
        }

        void DrawJudgements(
            const core::judgement::ScoreState& score,
            const double label_opacity,
            const double value_opacity
        ) {
            struct Row {
                AssetNameView texture;
                uint32 count;
                int32 texture_y;
                int32 value_y;
            };
            const std::array rows{
                Row{ assets::texture::ResultPerfect, score.perfect_count, 397, 385 },
                Row{ assets::texture::ResultGreat, score.great_count, 464, 452 },
                Row{ assets::texture::ResultGood, score.good_count, 530, 518 },
                Row{ assets::texture::ResultMiss, score.miss_count, 598, 586 },
            };
            for (const auto& row : rows) {
                TextureAsset{ row.texture }.draw(86, row.texture_y, ColorF{ 1.0, label_opacity });
                FontAsset{ assets::font::Text }(row.count)
                    .draw(48, Arg::topRight(500, row.value_y), ColorF{ 0.8, value_opacity });
            }
        }

        void DrawGrade(const uint32 score, const double frame_opacity, const double grade_opacity) {
            const Vec2 center{ 1050, 150 };
            TextureAsset{ assets::texture::ResultGradeBack }.draw(Arg::center = center, ColorF{ 1.0, frame_opacity });
            const auto label = presentation::GradeLabel(core::scoring::GradeForScore(score));
            const auto texture = label.front() == U'S'   ? assets::texture::ResultGradeS
                                 : label.front() == U'A' ? assets::texture::ResultGradeA
                                                         : assets::texture::ResultGradeB;
            for (std::size_t index = 0; index < label.size(); ++index) {
                const double offset = (static_cast<double>(index) - (label.size() - 1) * 0.5) * 60.0;
                TextureAsset{ texture }.draw(Arg::center = center.movedBy(offset, 0), ColorF{ 1.0, grade_opacity });
            }
        }
    }

    void DrawResultPanel(const ResultPanelData& data, const Point& position, const double elapsed) {
        const ScopedViewport2D viewport{ position, PanelSize };
        const double frame_opacity = Reveal(elapsed, 0.25, 0.25);
        const double value_opacity = Reveal(elapsed, 0.5, 0.25);
        const double score_progress = Reveal(elapsed, 0.5, 0.5);
        const double grade_opacity = Reveal(elapsed, 1.0, 0.25);
        const auto display = FontAsset{ assets::font::Display };
        const auto text = FontAsset{ assets::font::Text };

        DrawFrame(frame_opacity);
        display(U"SCORE").draw(40, Vec2{ 137, 42 }, ColorF{ 0.0, frame_opacity });
        DrawScore(data.score.score, value_opacity, score_progress);
        DrawJudgements(data.score, frame_opacity, value_opacity);
        DrawGrade(data.score.score, frame_opacity, grade_opacity);

        const auto clear_type = core::scoring::ClearTypeForScore(data.score, data.total_combo);
        const ColorF clear_color = clear_type == core::scoring::ClearType::Failed  ? theme::Palette::Gray
                                   : clear_type == core::scoring::ClearType::Clear ? theme::Palette::Cyan
                                                                                   : theme::Palette::Pink;
        display(presentation::ClearStatusLabel(clear_type))
            .drawAt(24, Vec2{ 1050, 290 }, clear_color.withA(value_opacity));

        display(U"COMBO").drawAt(40, Vec2{ 700, 410 }, ColorF{ 0.0, frame_opacity });
        text(data.score.max_combo).drawAt(80, Vec2{ 700, 500 }, theme::Palette::Pink.withA(value_opacity));

        Circle{ 1100, 500, 70 }.drawFrame(5, 0, theme::Palette::Purple.withA(0.7 * frame_opacity));
        display(U"RATE").drawAt(40, Vec2{ 1100, 410 }, ColorF{ 0.0, frame_opacity });
        const auto rating = presentation::ResultRatingAt(data.previous_rating, data.rating, elapsed);
        display(rating ? Format(*rating) : U"-").drawAt(70, Vec2{ 1100, 500 }, ColorF{ 0.0, frame_opacity });
        if (data.rating && data.previous_rating && data.previous_rating != data.rating) {
            const int64 difference = static_cast<int64>(*data.rating) - *data.previous_rating;
            const auto label = text(U"{:+}"_fmt(difference));
            const double font_size = Min(24.0, 24.0 * 140.0 / Max(1.0, label.region(24).w));
            label.drawAt(font_size, Vec2{ 1100, 610 }, theme::Palette::Purple.withA(Reveal(elapsed, 1.0, 0.25)));
        }
    }
}
