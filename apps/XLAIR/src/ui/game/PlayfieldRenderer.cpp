#include "PlayfieldRenderer.hpp"

#include "ui/assets/Assets.hpp"
#include "ui/primitives/Sparkle.hpp"

#include <cmath>

namespace xlair::ui::game {
    namespace {
        constexpr Size FieldSize{ 1000, 4000 };
        constexpr double FieldWidth = FieldSize.x;
        constexpr double FieldHeight = FieldSize.y;
        constexpr double StageLeft = 32.0;
        constexpr double LaneWidth = (FieldWidth - StageLeft * 2.0) / 16.0;
        constexpr double JudgeY = FieldHeight - 100.0;
        constexpr double NoteHeight = 32.0;
        // Keep the existing note-speed setting comparable to the former 880 px flat viewport.
        constexpr double ProjectionScale = FieldHeight / 880.0;

        [[nodiscard]]
        BlendState MaxAlphaBlend() {
            BlendState blend = BlendState::Default2D;
            blend.opAlpha = BlendOp::Max;
            blend.srcAlpha = Blend::SrcAlpha;
            blend.dstAlpha = Blend::DestAlpha;
            return blend;
        }

        struct LaneRange {
            double start = 0.0;
            double width = 1.0;
        };

        [[nodiscard]]
        LaneRange SliderRange(const sheets::LaneSpan lane) {
            return { .start = static_cast<double>(lane.start), .width = static_cast<double>(lane.width) };
        }

        [[nodiscard]]
        double NoteY(
            const playfield::ChartProjection& projection,
            const sheets::TimelineIndex timeline,
            const int64 current_sample,
            const int64 note_sample,
            const double pixels_per_second
        ) {
            return JudgeY - static_cast<double>(projection.noteDistance(timeline, current_sample, note_sample)) *
                                pixels_per_second * ProjectionScale;
        }

        [[nodiscard]]
        RectF LaneRect(const LaneRange range, const double y, const double height) {
            return { StageLeft + range.start * LaneWidth, y, range.width * LaneWidth, height };
        }

        [[nodiscard]]
        bool Visible(const double y) {
            return std::isfinite(y) && -NoteHeight <= y && y <= FieldHeight + NoteHeight;
        }

        void DrawNote(const RectF& rect, const ColorF& light, const ColorF& dark, const bool framed) {
            // Rotate the primitive to reproduce the legacy plate's horizontal gradient.
            const Vec2 left_center{ rect.x, rect.y + rect.h * 0.5 };
            const Transformer2D transform{ Mat3x2::Rotate(-90_deg, left_center) };
            const RoundRect plate =
                RoundRect{ left_center.movedBy(-rect.h * 0.5, 0), rect.h, rect.w, 5.0 }.stretched(0, -5);
            plate.drawShadow(Vec2{ -10, 0 }, 30.0, 0.0, dark.withA(0.5)).draw(Arg::top = light, Arg::bottom = dark);
            if (framed) {
                plate.movedBy(10, 0).stretched(0, -5).drawFrame(2.0, Arg::top = dark, Arg::bottom = light);
            }
        }

        void DrawSliderNote(const RectF& rect, const sheets::SliderNoteKind kind) {
            switch (kind) {
                case sheets::SliderNoteKind::Tap:
                    DrawNote(rect, ColorF{ U"#FFFFFF" }, ColorF{ U"#BEE4EF" }, true);
                    return;
                case sheets::SliderNoteKind::XTap:
                    DrawNote(rect, ColorF{ U"#FEEFB4" }, ColorF{ U"#FEDF64" }, true);
                    return;
                case sheets::SliderNoteKind::Flick:
                    DrawNote(rect, ColorF{ U"#D5A1F0" }, ColorF{ U"#B429FA" }, false);
                    return;
            }
        }

        [[nodiscard]]
        Quad FieldQuad(const Rect& viewport) {
            const double scale_x = viewport.w / 1920.0;
            const double center_x = viewport.x + viewport.w * 0.5;
            return {
                Vec2{ center_x - 110.0 * scale_x, static_cast<double>(viewport.y) },
                Vec2{ center_x + 110.0 * scale_x, static_cast<double>(viewport.y) },
                Vec2{ center_x + 735.0 * scale_x, static_cast<double>(viewport.y + viewport.h) },
                Vec2{ center_x - 735.0 * scale_x, static_cast<double>(viewport.y + viewport.h) },
            };
        }

        void DrawFieldBackground(const uint32 combo) {
            const ColorF judge_color{ U"#79C8FF" };
            RectF{ 0, 0, FieldWidth, FieldHeight }.draw(ColorF{ U"#212830" });
            for (int32 lane = 4; lane < 16; lane += 4) {
                const double x = StageLeft + lane * LaneWidth;
                Line{ x, 0, x, FieldHeight }.draw(2.5, ColorF{ U"#FDFDFD" });
            }
            FontAsset{ assets::font::ComboNumber }(combo).drawAt(FieldWidth * 0.5, FieldHeight - 750.0, Palette::White);
            FontAsset{ assets::font::Display }(U"COMBO").drawAt(FieldWidth * 0.5, FieldHeight - 600.0, Palette::White);
            primitives::Sparkle(Vec2{ StageLeft - 2.0, JudgeY }, 200.0, 300.0, 0.0, 16.0)
                .asPolygon()
                .drawFrame(1.5, judge_color);
            primitives::Sparkle(Vec2{ FieldWidth - StageLeft + 2.0, JudgeY }, 200.0, 300.0, 0.0, 16.0)
                .asPolygon()
                .drawFrame(1.5, judge_color);
            RectF{ 0, JudgeY - 2.5, FieldWidth, 5.0 }
                .drawShadow(Vec2{ 0, 10 }, 30.0, 0.0, judge_color.withA(0.15))
                .draw(judge_color);
        }

        void DrawFieldDecorations() {
            RectF{ 0, 0, StageLeft, FieldHeight }.draw(ColorF{ U"#B4E6FF" });
            RectF{ FieldWidth - StageLeft, 0, StageLeft, FieldHeight }.draw(ColorF{ U"#B4E6FF" });
            RectF{ 0, 0, FieldWidth, FieldHeight / 1.75 }.draw(
                Arg::top = ColorF{ 1.0, 0.9 },
                Arg::bottom = ColorF{ 1.0, 0.0 }
            );
        }

        void DrawSliderHolds(
            const sheets::Chart& chart,
            const playfield::ChartProjection& projection,
            const int64 current_sample,
            const double pixels_per_second
        ) {
            for (const auto& hold : chart.slider_holds) {
                for (std::size_t index = 1; index < hold.points.size(); ++index) {
                    const auto& before = hold.points[index - 1];
                    const auto& after = hold.points[index];
                    const double before_y =
                        NoteY(projection, before.timeline, current_sample, before.sample, pixels_per_second);
                    const double after_y =
                        NoteY(projection, after.timeline, current_sample, after.sample, pixels_per_second);
                    if (!std::isfinite(before_y) || !std::isfinite(after_y) || (before_y < 0 && after_y < 0) ||
                        (before_y > FieldHeight && after_y > FieldHeight)) {
                        continue;
                    }

                    const RectF before_rect = LaneRect(SliderRange(before.lane), before_y, 0);
                    const RectF after_rect = LaneRect(SliderRange(after.lane), after_y, 0);
                    Quad{
                        Vec2{ before_rect.x + 4, before_y },
                        Vec2{ before_rect.x + before_rect.w - 4, before_y },
                        Vec2{ after_rect.x + after_rect.w - 4, after_y },
                        Vec2{ after_rect.x + 4, after_y },
                    }
                        .draw(ColorF{ U"#43A4F2" }.withA(0.52));
                }

                for (const auto& point : hold.points) {
                    if (point.kind == sheets::SliderHoldPointKind::Invisible) {
                        continue;
                    }
                    const double y = NoteY(projection, point.timeline, current_sample, point.sample, pixels_per_second);
                    if (Visible(y)) {
                        DrawNote(
                            LaneRect(SliderRange(point.lane), y - NoteHeight * 0.5, NoteHeight),
                            ColorF{ U"#B4E8FF" },
                            ColorF{ U"#43A4F2" },
                            false
                        );
                    }
                }
            }
        }
    }

    PlayfieldRenderer::PlayfieldRenderer() : m_field_texture{ FieldSize } {}

    void PlayfieldRenderer::draw(
        const sheets::Chart& chart,
        const playfield::ChartProjection& projection,
        const int64 current_sample,
        const Rect& viewport,
        const double pixels_per_second,
        const uint32 combo
    ) const {
        const Quad field = FieldQuad(viewport);

        {
            const ScopedRenderTarget2D target{ m_field_texture.clear(ColorF{ 0.0, 0.0 }) };
            const ScopedRenderStates2D blend{ MaxAlphaBlend() };
            DrawFieldBackground(combo);
            DrawSliderHolds(chart, projection, current_sample, pixels_per_second);
            for (const auto& note : chart.slider_notes) {
                const double y = NoteY(projection, note.timeline, current_sample, note.sample, pixels_per_second);
                if (Visible(y)) {
                    DrawSliderNote(LaneRect(SliderRange(note.lane), y - NoteHeight * 0.5, NoteHeight), note.kind);
                }
            }
            DrawFieldDecorations();
            Graphics2D::Flush();
            m_field_texture.resolve();
        }

        {
            const ScopedRenderStates2D sampler{ SamplerState::ClampAniso };
            Shader::QuadWarp(field, m_field_texture);
        }
    }
}
