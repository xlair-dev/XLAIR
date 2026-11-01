#include "TutorialFocus.hpp"

#include "PlayfieldRenderer3D.hpp"

#include <cmath>
#include <limits>

namespace xlair::ui::game {
    namespace {
        constexpr long double NoteHalfDepth = 0.025L;
        constexpr long double JudgeHalfDepth = 0.006L;

        [[nodiscard]]
        bool IsUpper(const sheets::SideButton button) {
            return button == sheets::SideButton::LeftUpper || button == sheets::SideButton::RightUpper;
        }

        [[nodiscard]]
        bool IsFinite(const Quad& region) {
            for (std::size_t index = 0; index < 4; ++index) {
                const auto& point = region.p(index);
                if (!std::isfinite(point.x) || !std::isfinite(point.y)) {
                    return false;
                }
            }
            return true;
        }

        void AppendRegion(Array<Quad>& regions, const Quad& region, const RectF& viewport, const double margin) {
            if (!IsFinite(region)) {
                return;
            }
            const auto bounds = region.boundingRect();
            if (!std::isfinite(bounds.w) || !std::isfinite(bounds.h) || bounds.w <= 0.0 || bounds.h <= 0.0) {
                return;
            }
            // Preserve the projected shape while leaving room for the note's outline and glow.
            const auto padded = region.scaledAt(
                bounds.center(),
                (bounds.w + margin * 2.0) / bounds.w,
                (bounds.h + margin * 2.0) / bounds.h
            );
            if (IsFinite(padded) && padded.boundingRect().intersects(viewport)) {
                regions.push_back(padded);
            }
        }
    }

    Array<Quad> TutorialFocusRegions(
        const BasicCamera3D& camera,
        const sheets::Chart& chart,
        const playfield::ChartProjection& projection,
        const app::flows::Tutorial::Guide& guide,
        const int64 music_sample,
        const double note_speed
    ) {
        Array<Quad> regions;
        const auto size = camera.getSceneSize();
        if (chart.sample_rate <= 0 || !std::isfinite(note_speed) || note_speed <= 0.0 ||
            !std::isfinite(guide.end_beat) || size.x <= 0 || size.y <= 0) {
            return regions;
        }
        const long double end = std::round(
            static_cast<long double>(guide.end_beat) * chart.sample_rate * 60.0L / app::flows::Tutorial::BPM
        );
        if (end <= static_cast<long double>(std::numeric_limits<int64>::min()) ||
            end >= static_cast<long double>(std::numeric_limits<int64>::max())) {
            return regions;
        }
        const int64 resume_sample = static_cast<int64>(end);
        const RectF viewport{ 0.0, 0.0, static_cast<double>(size.x), static_cast<double>(size.y) };
        Array<long double> positions;
        const auto frame = projection.frameAt(music_sample, positions);
        const auto append = [&](const Quad& region, const double margin) {
            AppendRegion(regions, region, viewport, margin);
        };
        const auto slider_head =
            [&](const sheets::TimelineIndex timeline, const int64 sample, const sheets::LaneSpan lane) {
                if (timeline >= projection.timelineCount()) {
                    return;
                }
                const auto distance = frame.noteDistance(timeline, sample);
                append(
                    PlayfieldRenderer3D::sliderRegion(
                        camera,
                        lane.start,
                        lane.width,
                        distance - NoteHalfDepth,
                        distance + NoteHalfDepth,
                        note_speed
                    ),
                    10.0
                );
            };

        using Lesson = app::flows::Tutorial::Lesson;
        if (guide.lesson == Lesson::SideUpper || guide.lesson == Lesson::SideLower) {
            const bool upper = guide.lesson == Lesson::SideUpper;
            Optional<int64> first_sample;
            for (const auto& note : chart.side_notes) {
                if (note.sample < resume_sample || IsUpper(note.button) != upper) {
                    continue;
                }
                if (first_sample && note.sample != *first_sample) {
                    break;
                }
                first_sample = note.sample;
                if (note.timeline >= projection.timelineCount()) {
                    continue;
                }
                const auto distance = frame.noteDistance(note.timeline, note.sample);
                append(
                    PlayfieldRenderer3D::sideRegion(
                        camera,
                        note.button,
                        distance - NoteHalfDepth,
                        distance + NoteHalfDepth,
                        note_speed
                    ),
                    10.0
                );
            }
            for (const auto button : {
                     upper ? sheets::SideButton::LeftUpper : sheets::SideButton::LeftLower,
                     upper ? sheets::SideButton::RightUpper : sheets::SideButton::RightLower,
                 }) {
                append(
                    PlayfieldRenderer3D::sideRegion(camera, button, -JudgeHalfDepth, JudgeHalfDepth, note_speed),
                    3.0
                );
            }
        } else {
            if (guide.lesson == Lesson::Hold) {
                for (std::size_t index = 0; index < chart.slider_holds.size(); ++index) {
                    const auto& hold = chart.slider_holds[index];
                    if (hold.points.isEmpty() || hold.points.front().sample < resume_sample) {
                        continue;
                    }
                    const auto& start = hold.points.front();
                    slider_head(start.timeline, start.sample, start.lane);
                    Array<playfield::HoldSection> sections;
                    for (const auto& span : frame.sliderHoldSpans(index)) {
                        frame.holdSections(span, sections);
                        for (const auto& section : sections) {
                            const auto before = PlayfieldRenderer3D::sliderRegion(
                                camera,
                                section.before.lane_start,
                                section.before.lane_width,
                                section.before_distance,
                                section.before_distance,
                                note_speed
                            );
                            const auto after = PlayfieldRenderer3D::sliderRegion(
                                camera,
                                section.after.lane_start,
                                section.after.lane_width,
                                section.after_distance,
                                section.after_distance,
                                note_speed
                            );
                            append(Quad{ before.p0, before.p1, after.p1, after.p0 }, 4.0);
                        }
                    }
                    break;
                }
            } else {
                Optional<int64> first_sample;
                for (const auto& note : chart.slider_notes) {
                    const bool matching = guide.lesson == Lesson::Flick ? note.kind == sheets::SliderNoteKind::Flick
                                                                        : note.kind == sheets::SliderNoteKind::Tap ||
                                                                              note.kind == sheets::SliderNoteKind::XTap;
                    if (note.sample < resume_sample || !matching) {
                        continue;
                    }
                    if (first_sample && note.sample != *first_sample) {
                        break;
                    }
                    first_sample = note.sample;
                    slider_head(note.timeline, note.sample, note.lane);
                }
            }
            append(
                PlayfieldRenderer3D::sliderRegion(camera, 0.0, 16.0, -JudgeHalfDepth, JudgeHalfDepth, note_speed),
                3.0
            );
        }
        return regions;
    }
}
