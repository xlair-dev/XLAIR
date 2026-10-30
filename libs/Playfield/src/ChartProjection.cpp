#include <Playfield/ChartProjection.hpp>

#include <stdexcept>

namespace xlair::playfield {
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

    ChartProjection::ChartProjection(const sheets::Chart& chart) {
        m_timelines.reserve(chart.timelines.size());
        for (const auto& timeline : chart.timelines) {
            m_timelines.emplace_back(timeline, chart.sample_rate);
        }

        if (m_timelines.isEmpty()) {
            m_timelines.emplace_back(sheets::Timeline{}, chart.sample_rate);
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
