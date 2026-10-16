#include "Compiler.hpp"

#include "HoldJudgement.hpp"
#include "SliderHoldCompiler.hpp"
#include "Timing.hpp"

#include <algorithm>
#include <map>
#include <span>
#include <vector>

namespace xlair::sheets::compilation {
    namespace {
        [[nodiscard]]
        SideButton ToSideButton(const CcSideButton button) {
            switch (button) {
                case CcSideButton_LeftUpper:
                    return SideButton::LeftUpper;
                case CcSideButton_LeftLower:
                    return SideButton::LeftLower;
                case CcSideButton_RightLower:
                    return SideButton::RightLower;
                case CcSideButton_RightUpper:
                    return SideButton::RightUpper;
            }
            throw std::invalid_argument{ "Unknown side button from ChartConverter." };
        }

        [[nodiscard]]
        s3d::Optional<SideButton> AirButton(const CcAirDirection direction) {
            switch (direction) {
                case CcAirDirection_UpperLeft:
                    return SideButton::LeftUpper;
                case CcAirDirection_LowerLeft:
                    return SideButton::LeftLower;
                case CcAirDirection_LowerRight:
                    return SideButton::RightLower;
                case CcAirDirection_UpperRight:
                    return SideButton::RightUpper;
                default:
                    return s3d::none;
            }
        }

        class Timelines {
        public:
            Timelines(Chart& chart, const CcChartView& source, const TimingMap& timing)
                : m_chart{ chart }, m_scrolls{ source.scrolls, source.scroll_count }, m_timing{ timing } {
                (void)get(nullptr);
            }

            [[nodiscard]]
            TimelineIndex get(const CcNote* note) {
                std::vector<std::size_t> key;
                for (std::size_t index = 0; index < m_scrolls.size(); ++index) {
                    const auto& scroll = m_scrolls[index];
                    bool applies = scroll.scope == CcScrollScope_Global;
                    if (note) {
                        switch (scroll.scope) {
                            case CcScrollScope_Global:
                                break;
                            case CcScrollScope_Group:
                                applies = note->speed_group == scroll.target;
                                break;
                            case CcScrollScope_Note:
                                applies = note->id == scroll.target;
                                break;
                            case CcScrollScope_Lane:
                                applies = note->lane.kind == CcLaneKind_Slider &&
                                          scroll.lane.kind == CcLaneKind_Slider &&
                                          note->lane.start < scroll.lane.start + scroll.lane.width &&
                                          scroll.lane.start < note->lane.start + note->lane.width;
                                break;
                        }
                    }
                    if (applies) {
                        key.push_back(index);
                    }
                }
                if (const auto found = m_indices.find(key); found != m_indices.end()) {
                    return found->second;
                }
                const auto index = m_chart.timelines.size();
                Timeline timeline;
                timeline.speed_changes.push_back({ .sample = m_timing.toSample({ 0, 1 }), .multiplier = 1.0 });
                for (const auto change : key) {
                    timeline.speed_changes.push_back(
                        { .sample = m_timing.toSample(m_scrolls[change].position),
                          .multiplier = m_scrolls[change].speed }
                    );
                }
                m_chart.timelines.push_back(std::move(timeline));
                m_indices.emplace(std::move(key), index);
                return index;
            }

        private:
            Chart& m_chart;
            std::span<const CcScroll> m_scrolls;
            const TimingMap& m_timing;
            std::map<std::vector<std::size_t>, TimelineIndex> m_indices;
        };
    }

    Result<Chart> Compile(const CcChartView& source, const ChartLoadOptions& options) {
        auto timing_result = TimingMap::Build(source, options);
        if (!timing_result) {
            Result<Chart> result;
            result.diagnostics = std::move(timing_result.diagnostics);
            return result;
        }
        const auto& timing = *timing_result;
        Chart chart;
        chart.sample_rate = options.sample_rate;
        chart.tempo_changes = timing.tempoChanges();
        for (const auto& scroll : std::span{ source.scrolls, source.scroll_count }) {
            if (scroll.duration.denominator != 0) {
                return Result<Chart>::makeError(U"Gradual scroll-speed changes are not supported by the game.");
            }
        }
        Timelines timelines{ chart, source, timing };
        const std::span notes{ source.notes, source.note_count };
        s3d::Array<bool> side_parents(notes.size(), false);
        for (const auto& note : notes) {
            if (note.kind == CcNoteKind_Air && AirButton(note.air_direction) && note.parent < notes.size() &&
                notes[note.parent].kind == CcNoteKind_XTap) {
                side_parents[note.parent] = true;
            }
        }
        for (const auto& note : notes) {
            if (side_parents[note.id]) {
                continue;
            }
            if (note.kind == CcNoteKind_Air) {
                const auto button = AirButton(note.air_direction);
                if (!button || note.parent >= notes.size() || !side_parents[note.parent]) {
                    return Result<Chart>::makeError(U"An AIR note cannot be represented as an XLAIR side X-tap.");
                }
                const auto& parent = notes[note.parent];
                chart.side_notes.push_back(
                    { .timeline = timelines.get(&parent),
                      .sample = timing.toSample(parent.position),
                      .button = *button,
                      .kind = SideNoteKind::XTap }
                );
                continue;
            }
            const auto timeline = timelines.get(&note);
            const auto sample = timing.toSample(note.position);
            if (note.kind == CcNoteKind_Tap || note.kind == CcNoteKind_XTap || note.kind == CcNoteKind_Flick) {
                if (note.lane.kind == CcLaneKind_Side) {
                    if (note.kind == CcNoteKind_Flick) {
                        return Result<Chart>::makeError(U"Side flicks are not supported by XLAIR.");
                    }
                    chart.side_notes.push_back(
                        { .timeline = timeline,
                          .sample = sample,
                          .button = ToSideButton(note.lane.button),
                          .kind = note.kind == CcNoteKind_XTap ? SideNoteKind::XTap : SideNoteKind::Tap }
                    );
                } else {
                    const auto kind =
                        note.kind == CcNoteKind_Tap
                            ? SliderNoteKind::Tap
                            : (note.kind == CcNoteKind_XTap ? SliderNoteKind::XTap : SliderNoteKind::Flick);
                    chart.slider_notes.push_back(
                        { .kind = kind,
                          .timeline = timeline,
                          .sample = sample,
                          .lane = { .start = note.lane.start, .width = note.lane.width } }
                    );
                }
                continue;
            }
            if (note.kind != CcNoteKind_Slide && note.kind != CcNoteKind_Hold) {
                return Result<Chart>::makeError(U"The chart contains a note kind unsupported by XLAIR.");
            }
            const std::span points{ note.points, note.point_count };
            if (points.size() < 2) {
                return Result<Chart>::makeError(U"A long note requires a start and an end.");
            }
            if (note.lane.kind == CcLaneKind_Side) {
                SideHold hold{ .button = ToSideButton(note.lane.button) };
                for (std::size_t index = 0; index < points.size(); ++index) {
                    const auto kind =
                        index == 0 ? SideHoldPointKind::Start
                                   : (index + 1 == points.size() ? SideHoldPointKind::End : SideHoldPointKind::Relay);
                    hold.points.push_back(
                        { .kind = kind, .timeline = timeline, .sample = timing.toSample(points[index].position) }
                    );
                    if (index > 0 && hold.points[index].sample <= hold.points[index - 1].sample) {
                        return Result<Chart>::makeError(
                            U"Side hold points must have strictly increasing sample positions."
                        );
                    }
                }
                auto samples = detail::GenerateHoldJudgeSamples(
                    hold.points.front().sample,
                    hold.points.back().sample,
                    options.sample_rate,
                    chart.tempo_changes
                );
                if (!samples) {
                    Result<Chart> result;
                    result.diagnostics = std::move(samples.diagnostics);
                    return result;
                }
                for (const auto& point : hold.points) {
                    samples->push_back(point.sample);
                }
                samples->sort();
                samples->erase(std::unique(samples->begin(), samples->end()), samples->end());
                hold.judge_samples = std::move(*samples);
                chart.side_holds.push_back(std::move(hold));
            } else {
                s3d::Array<SliderPathPoint> path;
                for (std::size_t index = 0; index < points.size(); ++index) {
                    const auto& point = points[index];
                    if (point.lane.kind != CcLaneKind_Slider) {
                        return Result<Chart>::makeError(U"Slider hold points must remain on the slider.");
                    }
                    auto kind = point.kind == CcPointKind_Control
                                    ? SliderPathPoint::Kind::Control
                                    : (point.kind == CcPointKind_Invisible ? SliderPathPoint::Kind::Invisible
                                                                           : SliderPathPoint::Kind::Visible);
                    if (index == 0) {
                        kind = SliderPathPoint::Kind::Start;
                    } else if (index + 1 == points.size()) {
                        kind = SliderPathPoint::Kind::End;
                    }
                    path.push_back(
                        { .kind = kind,
                          .timeline = timeline,
                          .sample = timing.toSample(point.position),
                          .lane = { .start = point.lane.start, .width = point.lane.width } }
                    );
                }
                auto hold = detail::CompileSliderHold(path, options.sample_rate, chart.tempo_changes);
                if (!hold) {
                    Result<Chart> result;
                    result.diagnostics = std::move(hold.diagnostics);
                    return result;
                }
                chart.slider_holds.push_back(std::move(*hold));
            }
        }
        chart.slider_notes.stable_sort_by([](const auto& left, const auto& right) {
            return left.sample < right.sample;
        });
        chart.side_notes.stable_sort_by([](const auto& left, const auto& right) {
            return left.sample < right.sample;
        });
        chart.slider_holds.stable_sort_by([](const auto& left, const auto& right) {
            return left.points.front().sample < right.points.front().sample;
        });
        chart.side_holds.stable_sort_by([](const auto& left, const auto& right) {
            return left.points.front().sample < right.points.front().sample;
        });
        chart.total_combo = chart.slider_notes.size() + chart.side_notes.size();
        for (const auto& hold : chart.slider_holds) {
            chart.total_combo += hold.judge_points.size();
        }
        for (const auto& hold : chart.side_holds) {
            chart.total_combo += hold.judge_samples.size();
        }
        return Result<Chart>{ std::move(chart) };
    }
}
