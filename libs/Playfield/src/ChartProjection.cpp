#include <Playfield/ChartProjection.hpp>

#include <stdexcept>

namespace xlair::playfield {
    ChartProjection::ChartProjection(const sheets::Chart& chart) {
        m_timelines.reserve(chart.timelines.size());
        for (const auto& timeline : chart.timelines) {
            m_timelines.emplace_back(timeline, chart.sample_rate);
        }

        if (m_timelines.isEmpty()) {
            m_timelines.emplace_back(sheets::Timeline{}, chart.sample_rate);
        }
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

    std::size_t ChartProjection::timelineCount() const noexcept {
        return m_timelines.size();
    }
}
