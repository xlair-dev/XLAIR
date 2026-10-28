#include "Compiler.hpp"
#include "Timing.hpp"
#include "SliderHoldCompiler.hpp"
#include "HoldJudgement.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <span>

namespace xlair::sheets::chart {
    namespace {
        SideButton ToButton(const CcSideButton button) {
            // The public enum orders differ. Never cast between them.
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
            return SideButton::LeftUpper;
        }

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

        bool IsXTap(const CcNoteKind kind) {
            return kind == CcNoteKind_XTap || kind == CcNoteKind_ExTap;
        }

        LaneSpan ToLane(const CcLane& lane) {
            return { .start = lane.start, .width = lane.width };
        }

        SliderHoldPointKind ToHoldPointKind(const CcPointKind kind) {
            switch (kind) {
                case CcPointKind_Control:
                case CcPointKind_Invisible:
                    return SliderHoldPointKind::Invisible;
                case CcPointKind_Visible:
                    return SliderHoldPointKind::Visible;
            }
            return SliderHoldPointKind::Visible;
        }

        using TimelineLookup = std::map<s3d::int64, TimelineIndex>;

        Result<TimelineLookup>
        BuildTimelines(const CcResult& parsed, const CcChartView& view, const TimingMap& timing, Chart& chart) {
            std::set<s3d::int64> groups{ -1 };
            for (const auto& note : std::span{ view.notes, view.note_count }) {
                groups.insert(note.speed_group);
                const auto* details = cc_result_note_details(&parsed, note.id);
                if (!details || details->point_count != note.point_count) {
                    return Result<TimelineLookup>::makeError(U"Inconsistent point details in ChartConverter IR.");
                }
                for (const auto& point : std::span{ details->points, details->point_count }) {
                    groups.insert(point.speed_group);
                }
            }
            for (const auto& scroll : std::span{ view.scrolls, view.scroll_count }) {
                if (scroll.duration.denominator != 0 ||
                    (scroll.scope != CcScrollScope_Group && scroll.scope != CcScrollScope_Global)) {
                    return Result<TimelineLookup>::makeError(
                        U"Duration, lane, and note-scoped scroll changes are not supported by XLAIR."
                    );
                }
                if (scroll.scope == CcScrollScope_Group) {
                    groups.insert(scroll.target);
                }
            }
            TimelineLookup lookup;
            for (const auto group : groups) {
                lookup.emplace(group, chart.timelines.size());
                Timeline timeline{ .speed_changes = { { .sample = timing.toSample({ 0, 1 }), .multiplier = 1.0 } } };
                double global_speed = 1.0;
                double group_speed = 1.0;
                for (const auto& scroll : std::span{ view.scrolls, view.scroll_count }) {
                    if (scroll.scope == CcScrollScope_Global) {
                        global_speed = scroll.speed;
                    } else if (group >= 0 && scroll.target == static_cast<s3d::uint32>(group)) {
                        group_speed = scroll.speed;
                    } else {
                        continue;
                    }
                    const double speed = global_speed * group_speed;
                    if (!std::isfinite(speed)) {
                        return Result<TimelineLookup>::makeError(U"Combined scroll speed must be finite.");
                    }
                    timeline.speed_changes.push_back(
                        { .sample = timing.toSample(scroll.position), .multiplier = speed }
                    );
                }
                chart.timelines.push_back(std::move(timeline));
            }
            return Result<TimelineLookup>{ std::move(lookup) };
        }

        using NoteIds = std::set<s3d::uint32>;

        Result<NoteIds> CompileSideXTaps(
            const std::span<const CcNote> notes,
            const TimingMap& timing,
            const TimelineLookup& timelines,
            Chart& chart
        ) {
            // AIR + XTap is one side note. Resolve stable parent IDs without
            // changing the borrowed IR, then suppress those parents on the slider.
            std::map<s3d::uint32, const CcNote*> notes_by_id;
            for (const auto& note : notes) {
                notes_by_id.emplace(note.id, &note);
            }
            NoteIds parents;
            for (const auto& note : notes) {
                if (note.kind != CcNoteKind_Air) {
                    continue;
                }
                const auto parent = notes_by_id.find(note.parent);
                const auto button = AirButton(note.air_direction);
                if (parent == notes_by_id.end() || !IsXTap(parent->second->kind) || !button ||
                    parent->second->position.numerator != note.position.numerator ||
                    parent->second->position.denominator != note.position.denominator) {
                    return Result<NoteIds>::makeError(
                        U"Only diagonal AIR records paired with a simultaneous XTap can be used as side XTap notes."
                    );
                }
                parents.insert(parent->first);
                chart.side_notes.push_back(
                    { .timeline = timelines.at(note.speed_group),
                      .sample = timing.toSample(note.position),
                      .button = *button,
                      .kind = SideNoteKind::XTap }
                );
            }
            return Result<NoteIds>{ std::move(parents) };
        }

        Result<SideHold> CompileSideHold(
            const CcNote& note,
            const CcNoteDetails& details,
            const TimingMap& timing,
            const TimelineLookup& timelines,
            const Chart& chart
        ) {
            const std::span points{ note.points, note.point_count };
            SideHold hold{ .button = ToButton(note.lane.button) };
            hold.points.reserve(points.size());
            for (std::size_t index = 0; index < points.size(); ++index) {
                hold.points.push_back(
                    { .kind = index == 0
                                  ? SideHoldPointKind::Start
                                  : (index + 1 == points.size() ? SideHoldPointKind::End : SideHoldPointKind::Relay),
                      .timeline = timelines.at(details.points[index].speed_group),
                      .sample = timing.toSample(points[index].position) }
                );
                if (index > 0 && hold.points[index].sample <= hold.points[index - 1].sample) {
                    return Result<SideHold>::makeError(
                        U"Side hold points must have strictly increasing sample positions."
                    );
                }
            }
            auto samples = detail::GenerateHoldJudgeSamples(
                hold.points.front().sample,
                hold.points.back().sample,
                chart.sample_rate,
                chart.tempo_changes
            );
            if (!samples) {
                Result<SideHold> result;
                result.diagnostics = std::move(samples.diagnostics);
                return result;
            }
            for (const auto& point : hold.points) {
                if (point.kind == SideHoldPointKind::Relay) {
                    samples->push_back(point.sample);
                }
            }
            samples->sort();
            samples->erase(std::unique(samples->begin(), samples->end()), samples->end());
            hold.judge_samples = std::move(*samples);
            return Result<SideHold>{ std::move(hold) };
        }

        Result<SliderHold> CompileSliderHold(
            const CcNote& note,
            const CcNoteDetails& details,
            const TimingMap& timing,
            const TimelineLookup& timelines,
            const Chart& chart
        ) {
            const std::span points{ note.points, note.point_count };
            s3d::Array<SliderHoldPoint> resolved;
            resolved.reserve(points.size());
            for (std::size_t index = 0; index < points.size(); ++index) {
                const auto& point = points[index];
                const auto kind =
                    index == 0 ? SliderHoldPointKind::Start
                               : (index + 1 == points.size() ? SliderHoldPointKind::End : ToHoldPointKind(point.kind));
                resolved.push_back(
                    { .kind = kind,
                      .timeline = timelines.at(details.points[index].speed_group),
                      .sample = timing.toSample(point.position),
                      .lane = ToLane(point.lane) }
                );
            }
            return detail::CompileSliderHold(resolved, chart.sample_rate, chart.tempo_changes);
        }
    }

    Result<Chart> Compile(const CcResult& parsed, const ChartLoadOptions& options) {
        const auto view = cc_result_chart(&parsed);
        const auto audio_offset = cc_result_metadata(&parsed).audio_offset_seconds;
        auto timing_options = options;
        if (audio_offset.has_value) {
            timing_options.offset_seconds -= audio_offset.value;
        }
        auto timing_result = TimingMap::Build({ view.tempos, view.tempo_count }, timing_options);
        if (!timing_result) {
            Result<Chart> result;
            result.diagnostics = std::move(timing_result.diagnostics);
            return result;
        }
        const auto& timing = *timing_result;
        Chart chart{ .sample_rate = options.sample_rate };
        chart.tempo_changes.push_back({ .sample = timing.toSample({ 0, 1 }), .bpm = 120.0 });
        for (const auto& tempo : std::span{ view.tempos, view.tempo_count }) {
            chart.tempo_changes.push_back({ .sample = timing.toSample(tempo.position), .bpm = tempo.bpm });
        }
        auto timelines = BuildTimelines(parsed, view, timing, chart);
        if (!timelines) {
            Result<Chart> result;
            result.diagnostics = std::move(timelines.diagnostics);
            return result;
        }
        const std::span notes{ view.notes, view.note_count };
        auto side_xtap_parents = CompileSideXTaps(notes, timing, *timelines, chart);
        if (!side_xtap_parents) {
            Result<Chart> result;
            result.diagnostics = std::move(side_xtap_parents.diagnostics);
            return result;
        }

        for (const auto& note : notes) {
            if (note.kind == CcNoteKind_Air || side_xtap_parents->contains(note.id)) {
                continue;
            }
            const auto timeline = timelines->at(note.speed_group);
            if (note.kind == CcNoteKind_Tap || IsXTap(note.kind) || note.kind == CcNoteKind_Flick) {
                if (note.lane.kind == CcLaneKind_Side) {
                    if (note.kind == CcNoteKind_Flick) {
                        return Result<Chart>::makeError(U"Side Flick notes are not supported.");
                    }
                    chart.side_notes.push_back(
                        { .timeline = timeline,
                          .sample = timing.toSample(note.position),
                          .button = ToButton(note.lane.button),
                          .kind = IsXTap(note.kind) ? SideNoteKind::XTap : SideNoteKind::Tap }
                    );
                } else {
                    chart.slider_notes.push_back(
                        { .kind = IsXTap(note.kind)
                                      ? SliderNoteKind::XTap
                                      : (note.kind == CcNoteKind_Flick ? SliderNoteKind::Flick : SliderNoteKind::Tap),
                          .timeline = timeline,
                          .sample = timing.toSample(note.position),
                          .lane = ToLane(note.lane) }
                    );
                }
                continue;
            }
            if (note.kind != CcNoteKind_Hold && note.kind != CcNoteKind_Slide) {
                return Result<Chart>::makeError(
                    s3d::Format(
                        U"Unsupported XLAIR IR note kind ",
                        static_cast<s3d::uint32>(note.kind),
                        U" (note ",
                        note.id,
                        U")."
                    )
                );
            }
            if (note.point_count < 2) {
                return Result<Chart>::makeError(U"A hold needs at least two points.");
            }
            // BuildTimelines validated the IR point details before compilation.
            const auto& details = *cc_result_note_details(&parsed, note.id);
            if (note.lane.kind == CcLaneKind_Side) {
                auto hold = CompileSideHold(note, details, timing, *timelines, chart);
                if (!hold) {
                    Result<Chart> result;
                    result.diagnostics = std::move(hold.diagnostics);
                    return result;
                }
                chart.side_holds.push_back(std::move(*hold));
            } else {
                auto hold = CompileSliderHold(note, details, timing, *timelines, chart);
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
