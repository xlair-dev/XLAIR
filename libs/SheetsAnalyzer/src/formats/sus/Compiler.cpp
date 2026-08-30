#include "Compiler.hpp"

#include "SliderHoldCompiler.hpp"
#include "Timing.hpp"

namespace xlair::sheets::formats::sus {
    namespace detail {
        s3d::Optional<SideButton> SideButtonFromSideLongLane(const s3d::uint8 lane) {
            switch (lane) {
                case 0:
                case 1:
                    return SideButton::LeftUpper;
                case 2:
                case 3:
                    return SideButton::LeftLower;
                case 12:
                case 13:
                    return SideButton::RightLower;
                case 14:
                case 15:
                    return SideButton::RightUpper;
                default:
                    return s3d::none;
            }
        }
    }

    namespace {
        struct SideHoldBuilder {
            SideButton button = SideButton::LeftUpper;
            s3d::Array<SideHoldPoint> points;
        };

        [[nodiscard]] s3d::Optional<SideButton> ToSideButton(const DirectionalKind direction) {
            switch (direction) {
                case DirectionalKind::LeftUp:
                    return SideButton::LeftUpper;
                case DirectionalKind::RightUp:
                    return SideButton::RightUpper;
                case DirectionalKind::LeftDown:
                    return SideButton::LeftLower;
                case DirectionalKind::RightDown:
                    return SideButton::RightLower;
                case DirectionalKind::Up:
                case DirectionalKind::Down:
                    return s3d::none;
            }

            return s3d::none;
        }

        [[nodiscard]] s3d::Optional<::xlair::sheets::SliderNoteKind> ToChartNoteKind(const SliderNoteKind kind) {
            // XLAIR assigns its three slider-note types to SUS Tap 1 through Tap 3.
            switch (kind) {
                case SliderNoteKind::Tap1:
                    return ::xlair::sheets::SliderNoteKind::Tap;
                case SliderNoteKind::Tap2:
                    return ::xlair::sheets::SliderNoteKind::XTap;
                case SliderNoteKind::Tap3:
                    return ::xlair::sheets::SliderNoteKind::Flick;
                case SliderNoteKind::Tap4:
                case SliderNoteKind::Tap5:
                case SliderNoteKind::Tap6:
                    return s3d::none;
            }

            return s3d::none;
        }

        [[nodiscard]] ::xlair::sheets::LaneSpan ToChartLane(const LaneSpan lane) {
            return {
                .start = lane.start,
                .width = lane.width,
            };
        }

        [[nodiscard]] s3d::Optional<::xlair::sheets::SideHoldPointKind>
        ToChartSideHoldPointKind(const SideLongPointKind kind) {
            switch (kind) {
                case SideLongPointKind::Start:
                    return ::xlair::sheets::SideHoldPointKind::Start;
                case SideLongPointKind::End:
                    return ::xlair::sheets::SideHoldPointKind::End;
                case SideLongPointKind::Relay:
                    return ::xlair::sheets::SideHoldPointKind::Relay;
            }

            return s3d::none;
        }

        [[nodiscard]] bool PositionComesBefore(const Position& left, const Position& right) {
            if (left.measure != right.measure) {
                return left.measure < right.measure;
            }

            const s3d::uint64 left_denominator = left.denominator == 0 ? 1 : left.denominator;
            const s3d::uint64 right_denominator = right.denominator == 0 ? 1 : right.denominator;
            return (static_cast<s3d::uint64>(left.numerator) * right_denominator) <
                   (static_cast<s3d::uint64>(right.numerator) * left_denominator);
        }

        using TimelineLookup = s3d::HashTable<TimelineId, TimelineIndex>;

        [[nodiscard]] s3d::Optional<TimelineIndex> ResolveTimeline(const s3d::Optional<TimelineId>& source,
                                                                   const TimelineLookup& lookup) {
            if (!source) {
                return 0;
            }

            const auto timeline = lookup.find(*source);
            if (timeline == lookup.end()) {
                return s3d::none;
            }
            return timeline->second;
        }

        // SideLong (#mmm2xy) is compiled as a channel-based state machine:
        //
        //   Start (1?) -> resolve button once from the start lane
        //              -> create active_side_holds[channel]
        //   Relay      -> append to the active hold for channel
        //   End        -> append and finalize the active hold
        //
        // Start lanes 0-1, 2-3, C-D, and E-F select LeftUpper, LeftLower, RightLower, and RightUpper respectively.
        // Relay/End lanes do not re-select the button.
        //
        // This builder is kept private to the compiler because sus::Document should describe SUS source data, while
        // Chart should describe the physical controls of the game.
        using ActiveSideHolds = s3d::HashTable<ChannelId, SideHoldBuilder>;

        [[nodiscard]] Result<s3d::Array<SideHold>> CompileSideHolds(const s3d::Array<SideLongPoint>& source_points,
                                                                    const TimingMap& timing,
                                                                    const TimelineLookup& timeline_lookup) {
            auto ordered_points = source_points;
            ordered_points.stable_sort_by([](const auto& left, const auto& right) {
                return PositionComesBefore(left.position, right.position);
            });

            ActiveSideHolds active_holds;
            s3d::Array<SideHold> side_holds;

            for (const auto& source : ordered_points) {
                const auto kind = ToChartSideHoldPointKind(source.kind);
                if (!kind) {
                    return Result<s3d::Array<SideHold>>::makeError(U"A SideLong point has an unknown kind.");
                }

                const auto timeline = ResolveTimeline(source.timeline, timeline_lookup);
                if (!timeline) {
                    return Result<s3d::Array<SideHold>>::makeError(
                        U"A SideLong point references an undefined hispeed definition.");
                }

                const SideHoldPoint point{
                    .kind = *kind,
                    .timeline = *timeline,
                    .sample = timing.toSample(source.position),
                };

                if (source.kind == SideLongPointKind::Start) {
                    if (active_holds.contains(source.channel)) {
                        return Result<s3d::Array<SideHold>>::makeError(
                            U"A SideLong channel starts before its previous hold ends.");
                    }

                    const auto button = detail::SideButtonFromSideLongLane(source.lane.start);
                    if (!button) {
                        return Result<s3d::Array<SideHold>>::makeError(
                            U"A SideLong Start point does not map to an XLAIR side button.");
                    }

                    active_holds[source.channel] = {
                        .button = *button,
                        .points = { point },
                    };
                    continue;
                }

                const auto active = active_holds.find(source.channel);
                if (active == active_holds.end()) {
                    if (source.kind == SideLongPointKind::Relay) {
                        return Result<s3d::Array<SideHold>>::makeError(
                            U"A SideLong Relay point has no active Start point on its channel.");
                    }
                    return Result<s3d::Array<SideHold>>::makeError(
                        U"A SideLong End point has no active Start point on its channel.");
                }

                active->second.points.push_back(point);
                if (source.kind == SideLongPointKind::End) {
                    side_holds.push_back({
                        .button = active->second.button,
                        .points = std::move(active->second.points),
                    });
                    active_holds.erase(active);
                }
            }

            if (!active_holds.empty()) {
                return Result<s3d::Array<SideHold>>::makeError(U"A SideLong channel is missing an End point.");
            }

            side_holds.stable_sort_by([](const auto& left, const auto& right) {
                return left.points.front().sample < right.points.front().sample;
            });
            return Result<s3d::Array<SideHold>>{ std::move(side_holds) };
        }
    }

    Result<Chart> Compile(const Document& document, const CompileOptions& options) {
        auto timing_result = TimingMap::Build(document, {
                                                            .sample_rate = options.sample_rate,
                                                            .offset_seconds = options.offset_seconds,
                                                        });
        if (!timing_result) {
            Result<Chart> result;
            result.diagnostics = std::move(timing_result.diagnostics);
            return result;
        }
        const auto& timing = *timing_result;

        Chart chart;
        chart.sample_rate = options.sample_rate;

        const Position origin;
        chart.tempo_changes.push_back({
            .sample = timing.toSample(origin),
            .bpm = 120.0,
        });
        for (const auto& change : document.bpm_changes) {
            chart.tempo_changes.push_back({
                .sample = timing.toSample(change.position),
                .bpm = document.bpm_definitions.at(change.definition),
            });
        }
        chart.tempo_changes.stable_sort_by([](const auto& left, const auto& right) {
            return left.sample < right.sample;
        });

        Timeline default_timeline;
        default_timeline.speed_changes.push_back({
            .sample = timing.toSample(origin),
            .multiplier = 1.0,
        });
        chart.timelines.push_back(std::move(default_timeline));

        s3d::Array<TimelineId> timeline_ids;
        timeline_ids.reserve(document.hispeed_definitions.size());
        for (const auto& [id, _definition] : document.hispeed_definitions) {
            timeline_ids.push_back(id);
        }
        timeline_ids.sort();

        TimelineLookup timeline_lookup;
        for (const auto id : timeline_ids) {
            const TimelineIndex index = chart.timelines.size();
            timeline_lookup[id] = index;

            Timeline timeline;
            timeline.speed_changes.push_back({
                .sample = timing.toSample(origin),
                .multiplier = 1.0,
            });
            for (const auto& change : document.hispeed_definitions.at(id).changes) {
                timeline.speed_changes.push_back({
                    .sample = timing.toSample(change.position),
                    .multiplier = change.multiplier,
                });
            }
            timeline.speed_changes.stable_sort_by([](const auto& left, const auto& right) {
                return left.sample < right.sample;
            });
            chart.timelines.push_back(std::move(timeline));
        }

        for (const auto& source : document.slider_notes) {
            const auto kind = ToChartNoteKind(source.kind);
            if (!kind) {
                return Result<Chart>::makeError(U"SUS short-note kinds 4 through 6 are not supported by XLAIR.");
            }

            const auto timeline = ResolveTimeline(source.timeline, timeline_lookup);
            if (!timeline) {
                return Result<Chart>::makeError(U"A slider note references an undefined hispeed definition.");
            }

            chart.slider_notes.push_back({
                .kind = *kind,
                .timeline = *timeline,
                .sample = timing.toSample(source.position),
                .lane = ToChartLane(source.lane),
            });
        }
        chart.slider_notes.stable_sort_by([](const auto& left, const auto& right) {
            return left.sample < right.sample;
        });

        auto slider_holds = detail::CompileSliderHolds(document.slider_hold_points, timing, timeline_lookup,
                                                       options.sample_rate, chart.tempo_changes);
        if (!slider_holds) {
            Result<Chart> result;
            result.diagnostics = std::move(slider_holds.diagnostics);
            return result;
        }
        chart.slider_holds = std::move(*slider_holds);

        for (const auto& source : document.directional_notes) {
            const auto button = ToSideButton(source.kind);
            if (!button) {
                return Result<Chart>::makeError(U"Up and down directional notes are not supported by XLAIR.");
            }

            const auto timeline = ResolveTimeline(source.timeline, timeline_lookup);
            if (!timeline) {
                return Result<Chart>::makeError(U"A directional note references an undefined hispeed definition.");
            }

            chart.side_notes.push_back({
                .timeline = *timeline,
                .sample = timing.toSample(source.position),
                .button = *button,
            });
        }
        chart.side_notes.stable_sort_by([](const auto& left, const auto& right) {
            return left.sample < right.sample;
        });

        auto side_holds = CompileSideHolds(document.side_long_points, timing, timeline_lookup);
        if (!side_holds) {
            Result<Chart> result;
            result.diagnostics = std::move(side_holds.diagnostics);
            return result;
        }
        chart.side_holds = std::move(*side_holds);

        chart.total_combo = chart.slider_notes.size() + chart.side_notes.size();
        for (const auto& slider_hold : chart.slider_holds) {
            chart.total_combo += slider_hold.judge_points.size();
        }
        for (const auto& side_hold : chart.side_holds) {
            chart.total_combo += side_hold.judge_samples.size();
        }

        return Result<Chart>{ std::move(chart) };
    }
}
