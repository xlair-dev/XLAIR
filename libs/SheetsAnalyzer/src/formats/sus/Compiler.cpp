#include "Compiler.hpp"

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
    }

    Result<Chart> Compile(const Document& document, const CompileOptions& options) {
        if (!document.slider_hold_points.isEmpty() || !document.side_long_points.isEmpty()) {
            return Result<Chart>::makeError(U"Slider Hold and SideLong compilation has not been migrated yet.");
        }

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

        chart.total_combo = chart.slider_notes.size() + chart.side_notes.size();

        return Result<Chart>{ std::move(chart) };
    }
}
