#include "SliderHoldCompiler.hpp"

#include "HoldJudgement.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace xlair::sheets::formats::sus {
    namespace {
        constexpr long double CurveSegmentsPerSecond = 20.0L;
        constexpr std::size_t MaximumGeneratedCurvePointCount = 1'000'000;
        constexpr long double LaneEdgeEpsilon = 1e-9L;
        constexpr s3d::int32 SliderLaneCount = 16;

        struct SourcePoint {
            SliderHoldPointKind kind = SliderHoldPointKind::Visible;
            TimelineIndex timeline = 0;
            s3d::int64 sample = 0;
            ::xlair::sheets::LaneSpan lane;
        };

        struct HoldBuilder {
            s3d::Array<SourcePoint> points;
        };

        struct CurveCoordinate {
            long double sample = 0.0L;
            long double center_lane = 0.0L;
        };

        struct CurveSegment {
            s3d::Array<CurveCoordinate> control_points;
            long double start_width = 1.0L;
            long double end_width = 1.0L;
            TimelineIndex timeline = 0;
        };

        using TimelineLookup = s3d::HashTable<TimelineId, TimelineIndex>;
        using ActiveHolds = s3d::HashTable<ChannelId, HoldBuilder>;

        [[nodiscard]]
        bool PositionComesBefore(const Position& left, const Position& right) {
            if (left.measure != right.measure) {
                return left.measure < right.measure;
            }

            const s3d::uint64 left_denominator = left.denominator == 0 ? 1 : left.denominator;
            const s3d::uint64 right_denominator = right.denominator == 0 ? 1 : right.denominator;
            return (static_cast<s3d::uint64>(left.numerator) * right_denominator) <
                   (static_cast<s3d::uint64>(right.numerator) * left_denominator);
        }

        [[nodiscard]]
        s3d::Optional<TimelineIndex> ResolveTimeline(const s3d::Optional<TimelineId>& source,
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

        [[nodiscard]]
        ::xlair::sheets::LaneSpan ToChartLane(const LaneSpan lane) {
            return {
                .start = lane.start,
                .width = lane.width,
            };
        }

        [[nodiscard]]
        s3d::Optional<::xlair::sheets::SliderHoldPointKind> ToChartAnchorKind(const SliderHoldPointKind kind) {
            switch (kind) {
                case SliderHoldPointKind::Start:
                    return ::xlair::sheets::SliderHoldPointKind::Start;
                case SliderHoldPointKind::End:
                    return ::xlair::sheets::SliderHoldPointKind::End;
                case SliderHoldPointKind::Visible:
                    return ::xlair::sheets::SliderHoldPointKind::Visible;
                case SliderHoldPointKind::Invisible:
                    return ::xlair::sheets::SliderHoldPointKind::Invisible;
                case SliderHoldPointKind::Control:
                    return s3d::none;
            }

            return s3d::none;
        }

        [[nodiscard]]
        long double CenterLane(const ::xlair::sheets::LaneSpan lane) {
            return static_cast<long double>(lane.start) + (static_cast<long double>(lane.width) / 2.0L);
        }

        [[nodiscard]]
        CurveCoordinate ToCurveCoordinate(const SourcePoint& point) {
            return {
                .sample = static_cast<long double>(point.sample),
                .center_lane = CenterLane(point.lane),
            };
        }

        [[nodiscard]]
        CurveCoordinate EvaluateBezier(const s3d::Array<CurveCoordinate>& control_points, const long double t) {
            auto work = control_points;
            for (std::size_t remaining = work.size(); remaining > 1; --remaining) {
                for (std::size_t index = 0; index + 1 < remaining; ++index) {
                    work[index].sample = s3d::Math::Lerp(work[index].sample, work[index + 1].sample, t);
                    work[index].center_lane = s3d::Math::Lerp(work[index].center_lane, work[index + 1].center_lane, t);
                }
            }
            return work.front();
        }

        [[nodiscard]]
        s3d::int64 RoundSample(const long double sample) {
            constexpr auto Minimum = static_cast<long double>(std::numeric_limits<s3d::int64>::min());
            constexpr auto Maximum = static_cast<long double>(std::numeric_limits<s3d::int64>::max());
            return static_cast<s3d::int64>(s3d::Clamp(std::round(sample), Minimum, Maximum));
        }

        [[nodiscard]]
        ::xlair::sheets::LaneSpan QuantizeLane(const long double center_lane, const long double width) {
            const long double left_edge = center_lane - (width / 2.0L);
            const long double right_edge = center_lane + (width / 2.0L);
            const auto left =
                s3d::Clamp(static_cast<s3d::int32>(std::floor(left_edge + LaneEdgeEpsilon)), 0, SliderLaneCount - 1);
            const auto right =
                s3d::Clamp(static_cast<s3d::int32>(std::ceil(right_edge - LaneEdgeEpsilon)), left + 1, SliderLaneCount);
            return {
                .start = static_cast<s3d::uint8>(left),
                .width = static_cast<s3d::uint8>(right - left),
            };
        }

        [[nodiscard]]
        long double ParameterAtSample(const CurveSegment& segment, const s3d::int64 sample) {
            const long double first_sample = segment.control_points.front().sample;
            const long double last_sample = segment.control_points.back().sample;
            if (sample <= first_sample) {
                return 0.0L;
            }
            if (sample >= last_sample) {
                return 1.0L;
            }

            // The ordered control-point times make the Bezier time coordinate monotonic.
            // Inverting it lets judgement use the exact curve instead of the generated rendering polyline.
            long double lower = 0.0L;
            long double upper = 1.0L;
            for (std::size_t iteration = 0; iteration < 64; ++iteration) {
                const long double middle = (lower + upper) / 2.0L;
                if (EvaluateBezier(segment.control_points, middle).sample < sample) {
                    lower = middle;
                } else {
                    upper = middle;
                }
            }
            return (lower + upper) / 2.0L;
        }

        [[nodiscard]]
        ::xlair::sheets::LaneSpan LaneAtSample(const CurveSegment& segment, const s3d::int64 sample) {
            const long double t = ParameterAtSample(segment, sample);
            const auto coordinate = EvaluateBezier(segment.control_points, t);
            const long double segment_duration =
                segment.control_points.back().sample - segment.control_points.front().sample;
            const long double time_ratio =
                (coordinate.sample - segment.control_points.front().sample) / segment_duration;
            const long double width = s3d::Math::Lerp(segment.start_width, segment.end_width, time_ratio);
            return QuantizeLane(coordinate.center_lane, width);
        }

        [[nodiscard]]
        Result<std::size_t> CurveSubdivisionCount(const CurveSegment& segment, const s3d::int64 sample_rate) {
            const long double duration_samples =
                segment.control_points.back().sample - segment.control_points.front().sample;
            if (duration_samples <= 0.0L) {
                return Result<std::size_t>::makeError(
                    U"A Slider Hold Bezier segment must end after its preceding anchor.");
            }

            const long double subdivisions = std::ceil((duration_samples * CurveSegmentsPerSecond) / sample_rate);
            if (!std::isfinite(subdivisions) || subdivisions > MaximumGeneratedCurvePointCount) {
                return Result<std::size_t>::makeError(
                    U"A Slider Hold Bezier segment requires too many generated points.");
            }
            // A controlled segment always needs at least one generated interior point, even when it is short.
            return Result<std::size_t>{ static_cast<std::size_t>(s3d::Max(subdivisions, 2.0L)) };
        }

        [[nodiscard]]
        Result<SliderHold> BuildHold(const HoldBuilder& builder, const s3d::int64 sample_rate,
                                     const s3d::Array<TempoChange>& tempo_changes) {
            SliderHold hold;
            s3d::Array<CurveSegment> curve_segments;
            s3d::Array<CurveCoordinate> pending_control_points;

            const auto& start = builder.points.front();
            hold.points.push_back({
                .kind = ::xlair::sheets::SliderHoldPointKind::Start,
                .timeline = start.timeline,
                .sample = start.sample,
                .lane = start.lane,
            });
            pending_control_points.push_back(ToCurveCoordinate(start));

            auto previous_anchor = start;
            for (std::size_t index = 1; index < builder.points.size(); ++index) {
                const auto& source = builder.points[index];
                if (source.kind == SliderHoldPointKind::Control) {
                    pending_control_points.push_back(ToCurveCoordinate(source));
                    continue;
                }

                const auto kind = ToChartAnchorKind(source.kind);
                if (!kind) {
                    return Result<SliderHold>::makeError(U"A Slider Hold point has an unknown anchor kind.");
                }
                if (source.sample <= previous_anchor.sample) {
                    return Result<SliderHold>::makeError(
                        U"Slider Hold anchors must be placed in strictly increasing time order.");
                }

                pending_control_points.push_back(ToCurveCoordinate(source));
                CurveSegment segment{
                    .control_points = pending_control_points,
                    .start_width = static_cast<long double>(previous_anchor.lane.width),
                    .end_width = static_cast<long double>(source.lane.width),
                    .timeline = previous_anchor.timeline,
                };
                curve_segments.push_back(segment);

                if (pending_control_points.size() > 2) {
                    const auto subdivision_count = CurveSubdivisionCount(segment, sample_rate);
                    if (!subdivision_count) {
                        Result<SliderHold> result;
                        result.diagnostics = subdivision_count.diagnostics;
                        return result;
                    }

                    for (std::size_t subdivision = 1; subdivision < *subdivision_count; ++subdivision) {
                        const long double t = static_cast<long double>(subdivision) / *subdivision_count;
                        const auto coordinate = EvaluateBezier(segment.control_points, t);
                        const auto generated_sample = RoundSample(coordinate.sample);
                        if (generated_sample <= hold.points.back().sample || generated_sample >= source.sample) {
                            continue;
                        }

                        const long double segment_duration =
                            segment.control_points.back().sample - segment.control_points.front().sample;
                        const long double time_ratio =
                            (coordinate.sample - segment.control_points.front().sample) / segment_duration;
                        const long double width = s3d::Math::Lerp(segment.start_width, segment.end_width, time_ratio);
                        hold.points.push_back({
                            .kind = ::xlair::sheets::SliderHoldPointKind::Invisible,
                            // Control points affect geometry only. The preceding real anchor supplies the
                            // rendering timeline until the next real anchor takes over.
                            .timeline = segment.timeline,
                            .sample = generated_sample,
                            .lane = QuantizeLane(coordinate.center_lane, width),
                        });
                    }
                }

                hold.points.push_back({
                    .kind = *kind,
                    .timeline = source.timeline,
                    .sample = source.sample,
                    .lane = source.lane,
                });
                previous_anchor = source;
                pending_control_points = { ToCurveCoordinate(source) };
            }

            // Rendering subdivision and judgement density are intentionally independent.
            // Changing the curve's visual precision therefore cannot change its combo count.
            auto judge_samples = detail::GenerateHoldJudgeSamples(start.sample, builder.points.back().sample,
                                                                  sample_rate, tempo_changes);
            if (!judge_samples) {
                Result<SliderHold> result;
                result.diagnostics = std::move(judge_samples.diagnostics);
                return result;
            }
            for (const auto& source : builder.points) {
                if (source.kind == SliderHoldPointKind::Visible) {
                    judge_samples->push_back(source.sample);
                }
            }

            judge_samples->sort();
            judge_samples->erase(std::unique(judge_samples->begin(), judge_samples->end()), judge_samples->end());
            for (const auto sample : *judge_samples) {
                const auto segment =
                    std::find_if(curve_segments.begin(), curve_segments.end(), [sample](const auto& candidate) {
                        return sample <= candidate.control_points.back().sample;
                    });
                if (segment == curve_segments.end()) {
                    return Result<SliderHold>::makeError(U"A Slider Hold judgement could not be mapped to its curve.");
                }
                hold.judge_points.push_back({
                    .sample = sample,
                    .lane = LaneAtSample(*segment, sample),
                });
            }

            return Result<SliderHold>{ std::move(hold) };
        }
    }

    Result<s3d::Array<SliderHold>>
    detail::CompileSliderHolds(const s3d::Array<::xlair::sheets::formats::sus::SliderHoldPoint>& source_points,
                               const TimingMap& timing, const TimelineLookup& timeline_lookup,
                               const s3d::int64 sample_rate, const s3d::Array<TempoChange>& tempo_changes) {
        const auto ordered_points = source_points.stable_sorted_by([](const auto& left, const auto& right) {
            return PositionComesBefore(left.position, right.position);
        });

        ActiveHolds active_holds;
        s3d::Array<SliderHold> slider_holds;

        for (const auto& source : ordered_points) {
            if (source.lane.width == 0 || source.lane.start >= SliderLaneCount ||
                (static_cast<s3d::int32>(source.lane.start) + source.lane.width) > SliderLaneCount) {
                return Result<s3d::Array<SliderHold>>::makeError(
                    U"Slider Hold points must have a positive width and fit within XLAIR's 16 slider lanes.");
            }

            const auto timeline = ResolveTimeline(source.timeline, timeline_lookup);
            if (!timeline) {
                return Result<s3d::Array<SliderHold>>::makeError(
                    U"A Slider Hold point references an undefined hispeed definition.");
            }

            const SourcePoint point{
                .kind = source.kind,
                .timeline = *timeline,
                .sample = timing.toSample(source.position),
                .lane = ToChartLane(source.lane),
            };

            if (source.kind == SliderHoldPointKind::Start) {
                if (active_holds.contains(source.channel)) {
                    return Result<s3d::Array<SliderHold>>::makeError(
                        U"A Slider Hold channel starts before its previous hold ends.");
                }

                active_holds[source.channel] = {
                    .points = { point },
                };
                continue;
            }

            const auto active = active_holds.find(source.channel);
            if (active == active_holds.end()) {
                if (source.kind == SliderHoldPointKind::End) {
                    return Result<s3d::Array<SliderHold>>::makeError(
                        U"A Slider Hold End point has no active Start point on its channel.");
                }
                return Result<s3d::Array<SliderHold>>::makeError(
                    U"A Slider Hold point has no active Start point on its channel.");
            }

            active->second.points.push_back(point);
            if (source.kind == SliderHoldPointKind::End) {
                auto hold = BuildHold(active->second, sample_rate, tempo_changes);
                if (!hold) {
                    Result<s3d::Array<SliderHold>> result;
                    result.diagnostics = std::move(hold.diagnostics);
                    return result;
                }
                slider_holds.push_back(std::move(*hold));
                active_holds.erase(active);
            }
        }

        if (!active_holds.empty()) {
            return Result<s3d::Array<SliderHold>>::makeError(U"A Slider Hold channel is missing an End point.");
        }

        slider_holds.stable_sort_by([](const auto& left, const auto& right) {
            return left.points.front().sample < right.points.front().sample;
        });
        return Result<s3d::Array<SliderHold>>{ std::move(slider_holds) };
    }
}
