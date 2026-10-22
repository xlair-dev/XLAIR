#include "GameplaySoundScheduler.hpp"

#include <algorithm>
#include <utility>

namespace xlair::ui::audio {
    void GameplaySoundScheduler::start(const sheets::Chart& chart) {
        clear();
        for (const auto& note : chart.slider_notes) {
            switch (note.kind) {
                case sheets::SliderNoteKind::Tap:
                    m_slider_notes.push_back({ .sound = GameplaySoundKind::Tap, .sample = note.sample });
                    break;
                case sheets::SliderNoteKind::XTap:
                    m_slider_notes.push_back({ .sound = GameplaySoundKind::XTap, .sample = note.sample });
                    break;
                case sheets::SliderNoteKind::Flick:
                    m_slider_notes.push_back({ .sound = GameplaySoundKind::Flick, .sample = note.sample });
                    break;
            }
        }
        for (const auto& note : chart.side_notes) {
            m_side_notes.push_back(
                {
                    .sound =
                        note.kind == sheets::SideNoteKind::XTap ? GameplaySoundKind::XTap : GameplaySoundKind::SideTap,
                    .sample = note.sample,
                }
            );
        }
        for (const auto& hold : chart.slider_holds) {
            Array<Optional<Cue>> sounds(hold.judge_points.size());
            if (!hold.judge_points.isEmpty()) {
                sounds.front() =
                    Cue{ .sound = GameplaySoundKind::SlideTap, .sample = hold.judge_points.front().sample };
                for (const auto& point : hold.points) {
                    if (point.kind != sheets::SliderHoldPointKind::Visible &&
                        point.kind != sheets::SliderHoldPointKind::End) {
                        continue;
                    }
                    const auto judge = std::lower_bound(
                        hold.judge_points.begin(),
                        hold.judge_points.end(),
                        point.sample,
                        [](const auto& candidate, const int64 sample) {
                            return candidate.sample < sample;
                        }
                    );
                    if (judge != hold.judge_points.end() && judge->sample == point.sample &&
                        judge != hold.judge_points.begin()) {
                        sounds[static_cast<std::size_t>(judge - hold.judge_points.begin())] =
                            Cue{ .sound = GameplaySoundKind::Checkpoint, .sample = point.sample };
                    }
                }
            }
            m_slider_holds.push_back(
                { .start_sample = hold.judge_points.isEmpty() ? 0 : hold.judge_points.front().sample,
                  .points = std::move(sounds) }
            );
        }
        for (const auto& hold : chart.side_holds) {
            Array<Optional<Cue>> sounds(hold.judge_samples.size());
            if (!hold.judge_samples.isEmpty()) {
                sounds.front() = Cue{ .sound = GameplaySoundKind::SideTap, .sample = hold.judge_samples.front() };
                for (const auto& point : hold.points) {
                    if (point.kind != sheets::SideHoldPointKind::Relay &&
                        point.kind != sheets::SideHoldPointKind::End) {
                        continue;
                    }
                    const auto judge =
                        std::lower_bound(hold.judge_samples.begin(), hold.judge_samples.end(), point.sample);
                    if (judge != hold.judge_samples.end() && *judge == point.sample &&
                        judge != hold.judge_samples.begin()) {
                        sounds[static_cast<std::size_t>(judge - hold.judge_samples.begin())] =
                            Cue{ .sound = GameplaySoundKind::Checkpoint, .sample = point.sample };
                    }
                }
            }
            m_side_holds.push_back(
                { .start_sample = hold.judge_samples.isEmpty() ? 0 : hold.judge_samples.front(),
                  .points = std::move(sounds) }
            );
        }
    }

    GameplaySoundScheduler::Frame GameplaySoundScheduler::update(
        const int64 music_sample,
        const core::judgement::JudgeEngine& judge,
        const std::span<const core::judgement::JudgementEvent> events
    ) {
        Frame frame;
        for (const auto& event : events) {
            if (event.grade == core::judgement::Grade::Miss || event.linked_start) {
                continue;
            }
            Optional<Cue> cue;
            switch (event.note_kind) {
                case core::judgement::NoteKind::Slider:
                    cue = m_slider_notes.at(event.note_index);
                    break;
                case core::judgement::NoteKind::Side:
                    cue = m_side_notes.at(event.note_index);
                    break;
                case core::judgement::NoteKind::SliderHold:
                    cue = m_slider_holds.at(event.note_index).points.at(event.point_index);
                    break;
                case core::judgement::NoteKind::SideHold:
                    cue = m_side_holds.at(event.note_index).points.at(event.point_index);
                    break;
            }
            if (cue) {
                m_pending.push(*cue);
            }
        }
        // Never use offset_samples here: a FLICK can finish late despite early
        // contact, and input offsets must not shift the music's sound timeline.
        while (!m_pending.empty() && m_pending.top().sample <= music_sample) {
            frame.one_shots.push_back(m_pending.top().sound);
            m_pending.pop();
        }
        for (std::size_t index = 0; index < m_slider_holds.size(); ++index) {
            const auto& hold = m_slider_holds[index];
            frame.slider_hold_active =
                frame.slider_hold_active || (music_sample >= hold.start_sample && judge.sliderHoldActive(index));
        }
        for (std::size_t index = 0; index < m_side_holds.size(); ++index) {
            const auto& hold = m_side_holds[index];
            frame.side_hold_active =
                frame.side_hold_active || (music_sample >= hold.start_sample && judge.sideHoldActive(index));
        }
        return frame;
    }

    void GameplaySoundScheduler::clear() {
        m_pending = {};
        m_slider_notes.clear();
        m_side_notes.clear();
        m_slider_holds.clear();
        m_side_holds.clear();
    }
}
