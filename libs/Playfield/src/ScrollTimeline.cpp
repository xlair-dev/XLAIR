#include <Playfield/ScrollTimeline.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace xlair::playfield {
    ScrollTimeline::ScrollTimeline(const sheets::Timeline& timeline, const s3d::int64 sample_rate)
        : m_sample_rate{ sample_rate } {
        if (m_sample_rate <= 0) {
            throw std::invalid_argument{ "The sample rate must be a positive integer." };
        }

        auto changes = timeline.speed_changes;
        changes.stable_sort_by([](const sheets::SpeedChange& left, const sheets::SpeedChange& right) {
            return left.sample < right.sample;
        });

        for (const auto& change : changes) {
            if (!std::isfinite(change.multiplier)) {
                throw std::invalid_argument{ "Speed multipliers must be finite." };
            }

            if (m_segments.isEmpty()) {
                m_segments.push_back(
                    {
                        .sample = change.sample,
                        .multiplier = change.multiplier,
                        .position = 0.0L,
                    }
                );
                continue;
            }

            auto& previous = m_segments.back();
            if (previous.sample == change.sample) {
                // When changes share a sample, the last declaration becomes active.
                previous.multiplier = change.multiplier;
                continue;
            }

            const long double elapsed_samples =
                static_cast<long double>(change.sample) - static_cast<long double>(previous.sample);
            const long double position = previous.position + (elapsed_samples * previous.multiplier / m_sample_rate);
            m_segments.push_back(
                {
                    .sample = change.sample,
                    .multiplier = change.multiplier,
                    .position = position,
                }
            );
        }
    }

    long double ScrollTimeline::positionAt(const s3d::int64 sample) const {
        if (m_segments.isEmpty()) {
            return static_cast<long double>(sample) / m_sample_rate;
        }

        const auto segment = std::upper_bound(
            m_segments.begin(),
            m_segments.end(),
            sample,
            [](const s3d::int64 value, const Segment& candidate) {
                return value < candidate.sample;
            }
        );

        if (segment == m_segments.begin()) {
            const auto& first = m_segments.front();
            const long double elapsed_samples =
                static_cast<long double>(sample) - static_cast<long double>(first.sample);
            return first.position + (elapsed_samples / m_sample_rate);
        }

        const auto& active = *std::prev(segment);
        const long double elapsed_samples = static_cast<long double>(sample) - static_cast<long double>(active.sample);
        return active.position + (elapsed_samples * active.multiplier / m_sample_rate);
    }

    long double ScrollTimeline::distanceBetween(const s3d::int64 from_sample, const s3d::int64 to_sample) const {
        return positionAt(to_sample) - positionAt(from_sample);
    }

    long double ScrollTimeline::noteDistance(const s3d::int64 current_sample, const s3d::int64 note_sample) const {
        if (note_sample < current_sample) {
            const long double elapsed_samples =
                static_cast<long double>(note_sample) - static_cast<long double>(current_sample);
            return elapsed_samples / m_sample_rate;
        }

        return distanceBetween(current_sample, note_sample);
    }
}
