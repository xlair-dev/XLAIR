#pragma once

#include "ScrollTimeline.hpp"

#include <span>

namespace xlair::playfield {
    class ChartProjection {
    public:
        // A non-owning view valid only while its projection and output buffer live,
        // and until that buffer is modified or reused by frameAt().
        class Frame {
        public:
            [[nodiscard]]
            long double noteDistance(sheets::TimelineIndex timeline, s3d::int64 note_sample) const;

            void noteDistanceBreakpoints(
                sheets::TimelineIndex timeline,
                s3d::int64 from_sample,
                s3d::int64 to_sample,
                s3d::Array<s3d::int64>& samples
            ) const;

        private:
            friend class ChartProjection;

            Frame(
                const ChartProjection& projection,
                s3d::int64 current_sample,
                std::span<const long double> current_positions
            );

            const ChartProjection& m_projection;
            s3d::int64 m_current_sample;
            std::span<const long double> m_current_positions;
        };

        explicit ChartProjection(const sheets::Chart& chart);

        [[nodiscard]]
        Frame frameAt(s3d::int64 current_sample, s3d::Array<long double>& current_positions) const&;
        Frame frameAt(s3d::int64 current_sample, s3d::Array<long double>& current_positions) const&& = delete;

        [[nodiscard]]
        long double positionAt(sheets::TimelineIndex timeline, s3d::int64 sample) const;

        [[nodiscard]]
        long double distanceBetween(sheets::TimelineIndex timeline, s3d::int64 from_sample, s3d::int64 to_sample) const;

        // Returns a note's signed distance from the judgement line, using 1.0x
        // after the note has passed it.
        [[nodiscard]]
        long double
        noteDistance(sheets::TimelineIndex timeline, s3d::int64 current_sample, s3d::int64 note_sample) const;

        [[nodiscard]]
        s3d::Array<s3d::int64> noteDistanceBreakpoints(
            sheets::TimelineIndex timeline,
            s3d::int64 current_sample,
            s3d::int64 from_sample,
            s3d::int64 to_sample
        ) const;

        // Replaces the output while retaining its allocated storage.
        void noteDistanceBreakpoints(
            sheets::TimelineIndex timeline,
            s3d::int64 current_sample,
            s3d::int64 from_sample,
            s3d::int64 to_sample,
            s3d::Array<s3d::int64>& samples
        ) const;

        [[nodiscard]]
        std::size_t timelineCount() const noexcept;

    private:
        s3d::Array<ScrollTimeline> m_timelines;
    };
}
