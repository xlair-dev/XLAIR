#include <Playfield/ChartProjection.hpp>

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace xlair::playfield {
    namespace {
        [[nodiscard]]
        double ColorProgress(const s3d::int64 sample, const s3d::int64 first_sample, const double duration) {
            return duration > 0.0 ? s3d::Clamp((static_cast<double>(sample) - first_sample) / duration, 0.0, 1.0) : 0.0;
        }

        template <class Point>
        [[nodiscard]]
        PreparedHoldPoint PrepareHoldPoint(
            const ChartProjection& projection,
            const Point& before,
            const Point& after,
            const s3d::int64 sample,
            const sheets::TimelineIndex timeline,
            const s3d::int64 first_sample,
            const double duration
        ) {
            PreparedHoldPoint point{
                .sample = sample,
                .timeline = timeline,
                .position = projection.positionAt(timeline, sample),
                .color_progress = ColorProgress(sample, first_sample, duration),
            };
            if constexpr (requires {
                              before.lane;
                              after.lane;
                          }) {
                const double ratio = static_cast<double>(
                    (static_cast<long double>(sample) - before.sample) /
                    (static_cast<long double>(after.sample) - before.sample)
                );
                point.lane_start = s3d::Math::Lerp(
                    static_cast<double>(before.lane.start),
                    static_cast<double>(after.lane.start),
                    ratio
                );
                point.lane_width = s3d::Math::Lerp(
                    static_cast<double>(before.lane.width),
                    static_cast<double>(after.lane.width),
                    ratio
                );
            }
            return point;
        }

        template <class Point>
        [[nodiscard]]
        s3d::Array<PreparedHoldSpan>
        PrepareHoldSpans(const ChartProjection& projection, const s3d::Array<Point>& points) {
            s3d::Array<PreparedHoldSpan> spans;
            if (points.size() < 2) {
                return spans;
            }
            spans.reserve(points.size() - 1);
            const s3d::int64 first_sample = points.front().sample;
            const double duration = static_cast<double>(points.back().sample) - static_cast<double>(first_sample);
            s3d::Array<s3d::int64> samples;
            for (std::size_t index = 1; index < points.size(); ++index) {
                const auto& before = points[index - 1];
                const auto& after = points[index];
                PreparedHoldSpan span{
                    .before = { .sample = before.sample, .timeline = before.timeline },
                    .after = { .sample = after.sample, .timeline = after.timeline },
                    .first_sample = first_sample,
                    .duration = duration,
                };
                if (before.sample < after.sample) {
                    span.before = PrepareHoldPoint(
                        projection,
                        before,
                        after,
                        before.sample,
                        before.timeline,
                        first_sample,
                        duration
                    );
                    span.after = PrepareHoldPoint(
                        projection,
                        before,
                        after,
                        after.sample,
                        after.timeline,
                        first_sample,
                        duration
                    );
                    projection
                        .noteDistanceBreakpoints(before.timeline, before.sample, before.sample, after.sample, samples);
                    span.breakpoints.reserve(samples.size());
                    for (const auto sample : samples) {
                        span.breakpoints.push_back(
                            PrepareHoldPoint(projection, before, after, sample, before.timeline, first_sample, duration)
                        );
                    }
                }
                spans.push_back(std::move(span));
            }
            return spans;
        }
    }

    ChartProjection::Frame::Frame(
        const ChartProjection& projection,
        const s3d::int64 current_sample,
        const std::span<const long double> current_positions
    )
        : m_projection{ projection }, m_current_sample{ current_sample }, m_current_positions{ current_positions } {}

    long double
    ChartProjection::Frame::noteDistance(const sheets::TimelineIndex timeline, const s3d::int64 note_sample) const {
        if (note_sample < m_current_sample) {
            return m_projection.noteDistance(timeline, m_current_sample, note_sample);
        }
        const long double position = m_projection.positionAt(timeline, note_sample);
        return position - m_current_positions[timeline];
    }

    void ChartProjection::Frame::noteDistanceBreakpoints(
        const sheets::TimelineIndex timeline,
        const s3d::int64 from_sample,
        const s3d::int64 to_sample,
        s3d::Array<s3d::int64>& samples
    ) const {
        m_projection.noteDistanceBreakpoints(timeline, m_current_sample, from_sample, to_sample, samples);
    }

    std::span<const PreparedHoldSpan> ChartProjection::Frame::sliderHoldSpans(const std::size_t index) const {
        const auto& spans = m_projection.m_slider_hold_spans.at(index);
        return { spans.data(), spans.size() };
    }

    std::span<const PreparedHoldSpan> ChartProjection::Frame::sideHoldSpans(const std::size_t index) const {
        const auto& spans = m_projection.m_side_hold_spans.at(index);
        return { spans.data(), spans.size() };
    }

    void ChartProjection::Frame::holdSections(
        const PreparedHoldSpan& span,
        s3d::Array<HoldSection>& sections,
        const long double minimum_distance,
        const long double maximum_distance
    ) const {
        sections.clear();
        if (span.before.sample >= span.after.sample) {
            return;
        }
        const auto distance = [this](const PreparedHoldPoint& point) {
            if (point.sample < m_current_sample) {
                return m_projection.noteDistance(point.timeline, m_current_sample, point.sample);
            }
            if (point.timeline >= m_current_positions.size()) {
                throw std::out_of_range{ "The timeline index is outside the chart projection." };
            }
            return point.position - m_current_positions[point.timeline];
        };
        PreparedHoldPoint previous = span.before;
        long double previous_distance = distance(previous);
        const long double after_distance = distance(span.after);
        const bool crosses_current = span.before.sample < m_current_sample && m_current_sample < span.after.sample;
        if (minimum_distance <= maximum_distance &&
            (span.breakpoints.isEmpty() || span.breakpoints.back().sample <= m_current_sample)) {
            // Without future speed boundaries the only interior vertex can be the judgement line.
            if (previous_distance > maximum_distance && after_distance > maximum_distance &&
                (!crosses_current || 0.0L > maximum_distance)) {
                return;
            }
            if (previous_distance < minimum_distance && after_distance < minimum_distance &&
                (!crosses_current || 0.0L < minimum_distance)) {
                return;
            }
        }
        const auto append = [&](const PreparedHoldPoint& point, const long double point_distance) {
            sections.push_back(
                {
                    .before = previous,
                    .after = point,
                    .before_distance = previous_distance,
                    .after_distance = point_distance,
                }
            );
            previous = point;
            previous_distance = point_distance;
        };
        if (crosses_current) {
            if (span.before.timeline >= m_current_positions.size()) {
                throw std::out_of_range{ "The timeline index is outside the chart projection." };
            }
            const double ratio = static_cast<double>(
                (static_cast<long double>(m_current_sample) - span.before.sample) /
                (static_cast<long double>(span.after.sample) - span.before.sample)
            );
            const PreparedHoldPoint current{
                .sample = m_current_sample,
                .timeline = span.before.timeline,
                .position = m_current_positions[span.before.timeline],
                .lane_start = s3d::Math::Lerp(span.before.lane_start, span.after.lane_start, ratio),
                .lane_width = s3d::Math::Lerp(span.before.lane_width, span.after.lane_width, ratio),
                .color_progress = ColorProgress(m_current_sample, span.first_sample, span.duration),
            };
            append(current, distance(current));
        }
        const auto first = std::upper_bound(
            span.breakpoints.begin(),
            span.breakpoints.end(),
            std::max(span.before.sample, m_current_sample),
            [](const s3d::int64 sample, const PreparedHoldPoint& point) {
                return sample < point.sample;
            }
        );
        for (auto point = first; point != span.breakpoints.end(); ++point) {
            append(*point, distance(*point));
        }
        append(span.after, after_distance);
    }

    ChartProjection::ChartProjection(const sheets::Chart& chart) {
        m_timelines.reserve(chart.timelines.size());
        for (const auto& timeline : chart.timelines) {
            m_timelines.emplace_back(timeline, chart.sample_rate);
        }

        if (m_timelines.isEmpty()) {
            m_timelines.emplace_back(sheets::Timeline{}, chart.sample_rate);
        }

        m_slider_hold_spans.reserve(chart.slider_holds.size());
        for (const auto& hold : chart.slider_holds) {
            m_slider_hold_spans.push_back(PrepareHoldSpans(*this, hold.points));
        }
        m_side_hold_spans.reserve(chart.side_holds.size());
        for (const auto& hold : chart.side_holds) {
            m_side_hold_spans.push_back(PrepareHoldSpans(*this, hold.points));
        }
    }

    ChartProjection::Frame
    ChartProjection::frameAt(const s3d::int64 current_sample, s3d::Array<long double>& current_positions) const& {
        current_positions.resize(m_timelines.size());
        for (std::size_t index = 0; index < m_timelines.size(); ++index) {
            current_positions[index] = m_timelines[index].positionAt(current_sample);
        }
        return Frame{ *this,
                      current_sample,
                      std::span<const long double>{ current_positions.data(), current_positions.size() } };
    }

    long double ChartProjection::positionAt(const sheets::TimelineIndex timeline, const s3d::int64 sample) const {
        if (timeline >= m_timelines.size()) {
            throw std::out_of_range{ "The timeline index is outside the chart projection." };
        }
        return m_timelines[timeline].positionAt(sample);
    }

    long double ChartProjection::distanceBetween(
        const sheets::TimelineIndex timeline,
        const s3d::int64 from_sample,
        const s3d::int64 to_sample
    ) const {
        if (timeline >= m_timelines.size()) {
            throw std::out_of_range{ "The timeline index is outside the chart projection." };
        }
        return m_timelines[timeline].distanceBetween(from_sample, to_sample);
    }

    long double ChartProjection::noteDistance(
        const sheets::TimelineIndex timeline,
        const s3d::int64 current_sample,
        const s3d::int64 note_sample
    ) const {
        if (timeline >= m_timelines.size()) {
            throw std::out_of_range{ "The timeline index is outside the chart projection." };
        }
        return m_timelines[timeline].noteDistance(current_sample, note_sample);
    }

    s3d::Array<s3d::int64> ChartProjection::noteDistanceBreakpoints(
        const sheets::TimelineIndex timeline,
        const s3d::int64 current_sample,
        const s3d::int64 from_sample,
        const s3d::int64 to_sample
    ) const {
        s3d::Array<s3d::int64> samples;
        noteDistanceBreakpoints(timeline, current_sample, from_sample, to_sample, samples);
        return samples;
    }

    void ChartProjection::noteDistanceBreakpoints(
        const sheets::TimelineIndex timeline,
        const s3d::int64 current_sample,
        const s3d::int64 from_sample,
        const s3d::int64 to_sample,
        s3d::Array<s3d::int64>& samples
    ) const {
        samples.clear();
        if (timeline >= m_timelines.size()) {
            throw std::out_of_range{ "The timeline index is outside the chart projection." };
        }
        m_timelines[timeline].noteDistanceBreakpoints(current_sample, from_sample, to_sample, samples);
    }

    std::size_t ChartProjection::timelineCount() const noexcept {
        return m_timelines.size();
    }
}
