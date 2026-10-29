#include "JudgeEngine.hpp"

#include <SheetsAnalyzer/SliderHoldPath.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace xlair::core::judgement {
    namespace {
        constexpr uint8 SliderLaneCount = 16;
        constexpr uint8 TouchZoneCount = SliderLaneCount * 2;
        constexpr uint8 SideButtonCount = 4;
        // Community tests suggest adjacent-lane presses do not count as a FLICK.
        // Allow two lanes beyond the note so narrow FLICKs can still be swiped.
        constexpr uint8 FlickMinimumLaneTravel = 2;
        constexpr uint8 FlickOuterLaneAllowance = 2;

        [[nodiscard]]
        bool PressedZone(const uint32 pressed, const uint8 zone) {
            return (pressed & (uint32{ 1 } << zone)) != 0;
        }

        [[nodiscard]]
        bool Contains(const sheets::LaneSpan span, const uint8 lane) {
            return span.start <= lane && lane < (static_cast<uint32>(span.start) + span.width);
        }

        [[nodiscard]]
        bool Overlaps(const sheets::LaneSpan left, const sheets::LaneSpan right) {
            return left.start < (static_cast<uint32>(right.start) + right.width) &&
                   right.start < (static_cast<uint32>(left.start) + left.width);
        }

        [[nodiscard]]
        bool PressedInSpan(const uint32 pressed, const sheets::LaneSpan span) {
            for (uint32 lane = span.start; lane < static_cast<uint32>(span.start) + span.width; ++lane) {
                if ((pressed & (uint32{ 3 } << (lane * 2))) != 0) {
                    return true;
                }
            }
            return false;
        }

        [[nodiscard]]
        bool ValidLane(const sheets::LaneSpan lane) {
            return lane.width != 0 && lane.start < SliderLaneCount &&
                   static_cast<uint32>(lane.start) + lane.width <= SliderLaneCount;
        }

        [[nodiscard]]
        bool ValidSideButton(const sheets::SideButton button) {
            return static_cast<uint8>(button) < SideButtonCount;
        }

        [[nodiscard]]
        bool SidePressed(const uint8 pressed, const sheets::SideButton button) {
            return (pressed & (uint8{ 1 } << static_cast<uint8>(button))) != 0;
        }

        [[nodiscard]]
        bool FlickMotionInReach(const sheets::LaneSpan span, const uint8 contact_zone, const uint8 motion_zone) {
            const int32 contact_lane = contact_zone / 2;
            const int32 motion_lane = motion_zone / 2;
            const int32 last_lane = static_cast<int32>(span.start) + span.width - 1;
            return std::abs(contact_lane - motion_lane) >= FlickMinimumLaneTravel &&
                   motion_lane >= static_cast<int32>(span.start) - FlickOuterLaneAllowance &&
                   motion_lane <= last_lane + FlickOuterLaneAllowance;
        }
    }

    JudgeEngine::JudgeEngine(const sheets::Chart& chart)
        : m_slider_note_count{ chart.slider_notes.size() }, m_side_note_count{ chart.side_notes.size() },
          m_total_combo{ chart.total_combo } {
        if (chart.sample_rate <= 0) {
            throw std::invalid_argument{ "Judgement requires a positive chart sample rate." };
        }

        m_perfect_window = static_cast<int64>(std::llround(chart.sample_rate / 30.0));
        m_great_window = static_cast<int64>(std::llround(chart.sample_rate / 15.0));
        m_good_window = static_cast<int64>(std::llround(chart.sample_rate / 10.0));
        // Provisional values from player measurements: contact +/-83 ms, slide +/-167 ms.
        m_flick_tap_window = static_cast<int64>(std::llround(chart.sample_rate / 12.0));
        m_flick_slide_window = static_cast<int64>(std::llround(chart.sample_rate / 6.0));
        m_hold_end_release_window = static_cast<int64>(std::llround(chart.sample_rate / 10.0));

        m_notes.reserve(
            chart.slider_notes.size() + chart.side_notes.size() + chart.slider_holds.size() + chart.side_holds.size()
        );
        for (std::size_t index = 0; index < chart.slider_notes.size(); ++index) {
            const auto& source = chart.slider_notes[index];
            if (!ValidLane(source.lane)) {
                throw std::invalid_argument{ "A slider note has an invalid lane span." };
            }
            m_notes.push_back(
                {
                    .chart_index = index,
                    .sample = source.sample,
                    .lane = source.lane,
                    .slider_kind = source.kind,
                }
            );
        }

        for (std::size_t index = 0; index < chart.side_notes.size(); ++index) {
            const auto& source = chart.side_notes[index];
            if (!ValidSideButton(source.button)) {
                throw std::invalid_argument{ "A side note has an invalid button." };
            }
            m_notes.push_back(
                {
                    .note_kind = NoteKind::Side,
                    .chart_index = index,
                    .sample = source.sample,
                    .side_button = source.button,
                    .side_kind = source.kind,
                }
            );
        }

        m_slider_holds.reserve(chart.slider_holds.size());
        for (std::size_t hold_index = 0; hold_index < chart.slider_holds.size(); ++hold_index) {
            const auto& source = chart.slider_holds[hold_index];
            if (source.judge_points.isEmpty()) {
                throw std::invalid_argument{ "A slider hold has no judgement points." };
            }
            Array<HoldPoint> points;
            points.reserve(source.judge_points.size());
            for (std::size_t point_index = 0; point_index < source.judge_points.size(); ++point_index) {
                const auto& point = source.judge_points[point_index];
                if (!ValidLane(point.lane) ||
                    (point_index > 0 && point.sample <= source.judge_points[point_index - 1].sample)) {
                    throw std::invalid_argument{ "A slider hold has invalid judgement points." };
                }
                points.push_back(
                    {
                        .hold_index = hold_index,
                        .point_index = point_index,
                        .sample = point.sample,
                        .lane = point.lane,
                    }
                );
                if (point_index > 0) {
                    m_hold_order.push_back({ .hold = hold_index, .point = point_index });
                }
            }
            const auto& start = points.front();
            m_notes.push_back(
                {
                    .note_kind = NoteKind::SliderHold,
                    .chart_index = hold_index,
                    .sample = start.sample,
                    .lane = start.lane,
                }
            );
            m_slider_holds.push_back({ .points = std::move(points), .path = source.points });
        }

        m_side_holds.reserve(chart.side_holds.size());
        for (std::size_t hold_index = 0; hold_index < chart.side_holds.size(); ++hold_index) {
            const auto& source = chart.side_holds[hold_index];
            if (!ValidSideButton(source.button) || source.judge_samples.isEmpty()) {
                throw std::invalid_argument{ "A side hold has an invalid button or no judgement samples." };
            }

            Array<HoldPoint> points;
            points.reserve(source.judge_samples.size());
            for (std::size_t point_index = 0; point_index < source.judge_samples.size(); ++point_index) {
                const int64 sample = source.judge_samples[point_index];
                if (point_index > 0 && sample <= source.judge_samples[point_index - 1]) {
                    throw std::invalid_argument{ "A side hold has unordered judgement samples." };
                }
                points.push_back(
                    {
                        .note_kind = NoteKind::SideHold,
                        .hold_index = hold_index,
                        .point_index = point_index,
                        .sample = sample,
                        .side_button = source.button,
                    }
                );
                if (point_index > 0) {
                    m_hold_order.push_back(
                        { .note_kind = NoteKind::SideHold, .hold = hold_index, .point = point_index }
                    );
                }
            }
            m_notes.push_back(
                {
                    .note_kind = NoteKind::SideHold,
                    .chart_index = hold_index,
                    .sample = source.judge_samples.front(),
                    .side_button = source.button,
                }
            );
            m_side_holds.push_back({ .points = std::move(points) });
        }

        m_hold_order.stable_sort_by([this](const HoldPointIndex left, const HoldPointIndex right) {
            return holdPoint(left).sample < holdPoint(right).sample;
        });

        m_order.resize(m_notes.size());
        std::iota(m_order.begin(), m_order.end(), 0);
        m_order.stable_sort_by([this](const std::size_t left, const std::size_t right) {
            return m_notes[left].sample < m_notes[right].sample;
        });
        linkXTapStarts();
        for (const std::size_t index : m_order) {
            const auto& note = m_notes[index];
            if (note.note_kind == NoteKind::Slider && note.slider_kind == sheets::SliderNoteKind::Flick) {
                m_flick_order.push_back(index);
            }
        }
    }

    void JudgeEngine::linkXTapStarts() {
        for (auto& start : m_notes) {
            if (start.note_kind != NoteKind::SliderHold && start.note_kind != NoteKind::SideHold) {
                continue;
            }
            const auto first = std::lower_bound(
                m_order.begin(),
                m_order.end(),
                start.sample,
                [this](const std::size_t index, const int64 sample) {
                    return m_notes[index].sample < sample;
                }
            );
            for (auto it = first; it != m_order.end() && m_notes[*it].sample == start.sample; ++it) {
                const auto& note = m_notes[*it];
                if (!note.isXTap()) {
                    continue;
                }
                const bool linked = start.note_kind == NoteKind::SliderHold
                                        ? note.note_kind == NoteKind::Slider && Overlaps(start.lane, note.lane)
                                        : note.note_kind == NoteKind::Side && start.side_button == note.side_button;
                if (linked) {
                    start.linked_xtap = *it;
                    break;
                }
            }
        }
    }

    Array<JudgementEvent> JudgeEngine::advance(
        const int64 current_sample,
        const std::span<const TouchEvent> touches,
        const std::span<const SideEvent> side_down,
        const uint32 slider_pressed,
        const uint8 side_pressed
    ) {
        Array<JudgementEvent> events;
        m_flick_contacts.clear();

        for (const auto& touch : touches) {
            if (touch.zone >= TouchZoneCount || touch.sample > current_sample) {
                continue;
            }

            processFlickTouch(touch, slider_pressed, events);
            if (touch.phase == TouchEvent::Phase::Up) {
                continue;
            }

            const uint8 lane = touch.zone / 2;
            Note* selected = nullptr;
            Grade selected_grade = Grade::Miss;
            long double closest_distance = std::numeric_limits<long double>::infinity();

            const long double first_sample = static_cast<long double>(touch.sample) - m_good_window;
            const auto first = std::lower_bound(
                m_order.begin(),
                m_order.end(),
                first_sample,
                [this](const std::size_t index, const long double value) {
                    return m_notes[index].sample < value;
                }
            );
            for (auto it = first; it != m_order.end(); ++it) {
                auto& note = m_notes[*it];
                const long double distance =
                    std::abs(static_cast<long double>(touch.sample) - static_cast<long double>(note.sample));
                if (note.sample > static_cast<long double>(touch.sample) + m_good_window) {
                    break;
                }
                if ((note.note_kind != NoteKind::Slider && note.note_kind != NoteKind::SliderHold) ||
                    (note.note_kind == NoteKind::Slider && note.slider_kind == sheets::SliderNoteKind::Flick) ||
                    note.state != NoteState::Pending || !Contains(note.lane, lane)) {
                    continue;
                }

                const Grade grade = gradeFor(note, touch.sample);
                if (grade == Grade::Miss || !acceptsBoundary(note, lane, touch.sample, grade)) {
                    continue;
                }
                if (distance < closest_distance) {
                    selected = &note;
                    selected_grade = grade;
                    closest_distance = distance;
                }
            }

            if (selected) {
                record(*selected, selected_grade, touch.sample, events);
            }
        }

        for (const auto& input : side_down) {
            if (input.sample > current_sample) {
                continue;
            }

            Note* selected = nullptr;
            long double closest_distance = std::numeric_limits<long double>::infinity();
            const long double first_sample = static_cast<long double>(input.sample) - m_good_window;
            const auto first = std::lower_bound(
                m_order.begin(),
                m_order.end(),
                first_sample,
                [this](const std::size_t index, const long double value) {
                    return m_notes[index].sample < value;
                }
            );
            for (auto it = first; it != m_order.end(); ++it) {
                auto& note = m_notes[*it];
                if (static_cast<long double>(note.sample) > static_cast<long double>(input.sample) + m_good_window) {
                    break;
                }
                if ((note.note_kind != NoteKind::Side && note.note_kind != NoteKind::SideHold) ||
                    note.side_button != input.button || note.state != NoteState::Pending) {
                    continue;
                }
                const long double distance = std::abs(static_cast<long double>(input.sample) - note.sample);
                if (distance < closest_distance && acceptsSideBoundary(note, input.sample)) {
                    selected = &note;
                    closest_distance = distance;
                }
            }
            if (selected) {
                record(*selected, gradeFor(*selected, input.sample), input.sample, events);
            }
        }

        updateSliderEndContacts(current_sample, touches, slider_pressed);

        // Starts use down-edge windows above. Later checkpoints use the held
        // slider lane or side button; each can recover after a previous miss.
        for (std::size_t cursor = m_hold_cursor; cursor < m_hold_order.size(); ++cursor) {
            const auto index = m_hold_order[cursor];
            auto& point = holdPoint(index);
            if (static_cast<long double>(point.sample) > static_cast<long double>(current_sample) + m_great_window) {
                break;
            }
            if (point.state != NoteState::Pending) {
                continue;
            }
            const auto& hold =
                (point.note_kind == NoteKind::SideHold ? m_side_holds : m_slider_holds)[point.hold_index];
            const bool slider_end = point.note_kind == NoteKind::SliderHold && index.point + 1 == hold.points.size();
            if (slider_end && current_sample < point.sample) {
                continue;
            }
            const long double distance = std::abs(static_cast<long double>(current_sample) - point.sample);
            const bool pressed = point.note_kind == NoteKind::SliderHold ? PressedInSpan(slider_pressed, point.lane)
                                                                         : SidePressed(side_pressed, point.side_button);
            // Resolve the tail at its chart time, remembering contact shortly before release.
            const bool released_near_end = slider_end && hold.last_end_contact_sample.has_value();
            if (released_near_end || (distance <= m_great_window && pressed)) {
                record(point, Grade::Perfect, current_sample, events);
            } else if ((static_cast<long double>(current_sample) - point.sample) > m_good_window) {
                record(point, Grade::Miss, current_sample, events);
            }
        }
        while (m_hold_cursor < m_hold_order.size()) {
            const auto index = m_hold_order[m_hold_cursor];
            if (holdPoint(index).state == NoteState::Pending) {
                break;
            }
            ++m_hold_cursor;
        }

        while (m_miss_cursor < m_order.size()) {
            auto& note = m_notes[m_order[m_miss_cursor]];
            if ((static_cast<long double>(current_sample) - note.sample) <= m_good_window) {
                break;
            }
            if (note.note_kind != NoteKind::Slider || note.slider_kind != sheets::SliderNoteKind::Flick) {
                if (note.state == NoteState::Pending) {
                    record(note, Grade::Miss, current_sample, events);
                }
            }
            ++m_miss_cursor;
        }
        while (m_flick_miss_cursor < m_flick_order.size()) {
            auto& note = m_notes[m_flick_order[m_flick_miss_cursor]];
            if ((static_cast<long double>(current_sample) - note.sample) <= m_flick_slide_window) {
                break;
            }
            if (note.state == NoteState::Pending) {
                // A touched FLICK without a completed horizontal gesture is a late ATTACK.
                record(
                    note,
                    note.flick_gesture ? Grade::Good : Grade::Miss,
                    note.sample + m_flick_slide_window,
                    events
                );
            }
            ++m_flick_miss_cursor;
        }
        m_previous_slider_pressed = slider_pressed;
        updateHoldActivity(current_sample, slider_pressed, side_pressed);
        return events;
    }

    Array<JudgementEvent> JudgeEngine::finish() {
        Array<JudgementEvent> events;
        m_flick_contacts.clear();
        // Keep advance()'s checkpoint/note/FLICK timeout order without inventing a playback time.
        for (const auto index : m_hold_order) {
            auto& point = holdPoint(index);
            if (point.state == NoteState::Pending) {
                record(point, Grade::Miss, point.sample, events);
            }
        }
        for (const auto index : m_order) {
            auto& note = m_notes[index];
            if ((note.note_kind != NoteKind::Slider || note.slider_kind != sheets::SliderNoteKind::Flick) &&
                note.state == NoteState::Pending) {
                record(note, Grade::Miss, note.sample, events);
            }
        }
        for (const auto index : m_flick_order) {
            auto& note = m_notes[index];
            if (note.state == NoteState::Pending) {
                // Preserve the timeout result for a touched FLICK without a completed gesture.
                const Grade grade = note.flick_gesture ? Grade::Good : Grade::Miss;
                constexpr auto maximum = std::numeric_limits<int64>::max();
                const int64 sample =
                    note.sample > maximum - m_flick_slide_window ? maximum : note.sample + m_flick_slide_window;
                record(note, grade, sample, events);
            }
        }
        m_previous_slider_pressed = 0;
        updateHoldActivity(0, 0, 0);
        return events;
    }

    Array<JudgementEvent> JudgeEngine::advanceAuto(const int64 current_sample) {
        Array<JudgementEvent> events;
        m_flick_contacts.clear();
        // Merge the sorted note/start and checkpoint streams. Each entry is
        // visited once, even if a slow frame crosses several chart events.
        while (true) {
            const auto* note = m_auto_note_cursor < m_order.size() ? &m_notes[m_order[m_auto_note_cursor]] : nullptr;
            const auto* point =
                m_auto_hold_cursor < m_hold_order.size() ? &holdPoint(m_hold_order[m_auto_hold_cursor]) : nullptr;
            const bool note_due = note && note->sample <= current_sample;
            const bool point_due = point && point->sample <= current_sample;
            if (!note_due && !point_due) {
                break;
            }
            // Stable note ordering puts a co-located XTap before its hold start.
            if (note_due && (!point_due || note->sample <= point->sample)) {
                auto& due = m_notes[m_order[m_auto_note_cursor++]];
                if (due.state == NoteState::Pending) {
                    record(due, Grade::Perfect, due.sample, events);
                }
            } else {
                auto& due = holdPoint(m_hold_order[m_auto_hold_cursor++]);
                if (due.state == NoteState::Pending) {
                    record(due, Grade::Perfect, due.sample, events);
                }
            }
        }
        updateHoldActivity(current_sample, std::numeric_limits<uint32>::max(), std::numeric_limits<uint8>::max());
        return events;
    }

    void JudgeEngine::processFlickTouch(
        const TouchEvent& touch,
        const uint32 slider_pressed,
        Array<JudgementEvent>& events
    ) {
        const long double first_sample = static_cast<long double>(touch.sample) - m_flick_slide_window;
        const auto first = std::lower_bound(
            m_flick_order.begin(),
            m_flick_order.end(),
            first_sample,
            [this](const std::size_t index, const long double value) {
                return m_notes[index].sample < value;
            }
        );

        Note* active = nullptr;
        long double closest_distance = std::numeric_limits<long double>::infinity();
        for (auto it = first; it != m_flick_order.end(); ++it) {
            auto& note = m_notes[*it];
            if (static_cast<long double>(note.sample) > static_cast<long double>(touch.sample) + m_flick_tap_window) {
                break;
            }
            if (note.state != NoteState::Pending || !note.flick_gesture) {
                continue;
            }
            auto& gesture = *note.flick_gesture;
            if (touch.sample < gesture.contact_sample ||
                (static_cast<long double>(touch.sample) - note.sample) > m_flick_slide_window) {
                continue;
            }
            if (gesture.motion_zone &&
                (static_cast<long double>(touch.sample) - gesture.motion_sample) > m_flick_tap_window) {
                gesture.motion_zone.reset();
                gesture.preheld_motion = false;
            }

            bool eligible = false;
            if (touch.phase == TouchEvent::Phase::Down && !gesture.motion_zone) {
                eligible = FlickMotionInReach(note.lane, gesture.contact_zone, touch.zone) &&
                           (static_cast<long double>(touch.sample) - gesture.contact_sample) <= m_flick_slide_window;
            } else if (touch.phase == TouchEvent::Phase::Up && gesture.motion_zone) {
                eligible = touch.zone == gesture.contact_zone ||
                           (!gesture.preheld_motion && touch.zone == *gesture.motion_zone);
                eligible = eligible && touch.sample >= gesture.motion_sample &&
                           (static_cast<long double>(touch.sample) - gesture.motion_sample) <= m_flick_tap_window;
            }
            const long double distance = std::abs(static_cast<long double>(touch.sample) - note.sample);
            if (eligible && distance < closest_distance) {
                active = &note;
                closest_distance = distance;
            }
        }

        if (active) {
            auto& gesture = *active->flick_gesture;
            if (touch.phase == TouchEvent::Phase::Down) {
                gesture.motion_zone = touch.zone;
                gesture.motion_sample = touch.sample;
                if (!PressedZone(slider_pressed, gesture.contact_zone)) {
                    record(*active, flickGrade(*active), gesture.contact_sample, events);
                }
            } else {
                record(*active, flickGrade(*active), gesture.contact_sample, events);
            }
        }

        if (touch.phase != TouchEvent::Phase::Down) {
            return;
        }

        Note* contact = nullptr;
        closest_distance = std::numeric_limits<long double>::infinity();
        for (auto it = first; it != m_flick_order.end(); ++it) {
            auto& note = m_notes[*it];
            if (static_cast<long double>(note.sample) > static_cast<long double>(touch.sample) + m_flick_tap_window) {
                break;
            }
            const long double distance = std::abs(static_cast<long double>(touch.sample) - note.sample);
            if (distance > m_flick_tap_window || note.state != NoteState::Pending || note.flick_gesture ||
                !Contains(note.lane, touch.zone / 2)) {
                continue;
            }
            const Grade grade = distance <= m_perfect_window ? Grade::Perfect : Grade::Great;
            if (distance < closest_distance && acceptsBoundary(note, touch.zone / 2, touch.sample, grade)) {
                contact = &note;
                closest_distance = distance;
            }
        }
        if (!contact) {
            return;
        }

        contact->flick_gesture = FlickGesture{ .contact_zone = touch.zone, .contact_sample = touch.sample };
        m_flick_contacts.push_back(contact->chart_index);
        // An already-held lane can be the other half of a two-point FLICK.
        // The newly touched lane must still be released to complete it.
        for (uint8 zone = 0; zone < TouchZoneCount; ++zone) {
            if (PressedZone(m_previous_slider_pressed, zone) && FlickMotionInReach(contact->lane, touch.zone, zone)) {
                auto& gesture = *contact->flick_gesture;
                gesture.motion_zone = zone;
                gesture.motion_sample = touch.sample;
                gesture.preheld_motion = true;
                break;
            }
        }
    }

    Grade JudgeEngine::flickGrade(const Note& note) const {
        const auto& gesture = *note.flick_gesture;
        const long double contact_distance = std::abs(static_cast<long double>(gesture.contact_sample) - note.sample);
        const long double motion_distance = std::abs(static_cast<long double>(gesture.motion_sample) - note.sample);
        return contact_distance <= m_perfect_window && motion_distance <= m_flick_tap_window ? Grade::Perfect
                                                                                             : Grade::Great;
    }

    NoteState JudgeEngine::noteState(const std::size_t slider_note_index) const {
        return m_notes.at(slider_note_index).state;
    }

    NoteState JudgeEngine::sideNoteState(const std::size_t side_note_index) const {
        return m_notes.at(m_slider_note_count + side_note_index).state;
    }

    NoteState JudgeEngine::sliderHoldPointState(const std::size_t hold_index, const std::size_t point_index) const {
        return m_slider_holds.at(hold_index).points.at(point_index).state;
    }

    NoteState JudgeEngine::sideHoldPointState(const std::size_t hold_index, const std::size_t point_index) const {
        return m_side_holds.at(hold_index).points.at(point_index).state;
    }

    bool JudgeEngine::sliderHoldActive(const std::size_t hold_index) const {
        return m_slider_holds.at(hold_index).active;
    }

    bool JudgeEngine::sideHoldActive(const std::size_t hold_index) const {
        return m_side_holds.at(hold_index).active;
    }

    void JudgeEngine::updateSliderEndContacts(
        const int64 sample,
        const std::span<const TouchEvent> touches,
        const uint32 slider_pressed
    ) {
        for (auto& hold : m_slider_holds) {
            const auto& end = hold.points.back();
            if (!hold.acquired || end.state != NoteState::Pending ||
                static_cast<long double>(sample) < static_cast<long double>(end.sample) - m_hold_end_release_window) {
                continue;
            }
            const auto lane_at = [&hold, &end](const int64 contact_sample) {
                return hold.path.isEmpty() ? Optional<sheets::LaneSpan>{ end.lane }
                                           : sheets::EvaluateSliderHoldLane(hold.path, contact_sample);
            };
            const auto lane = lane_at(sample);
            if (sample <= end.sample && lane && PressedInSpan(slider_pressed, *lane)) {
                hold.last_end_contact_sample = sample;
            }
            for (const auto& touch : touches) {
                if (touch.phase == TouchEvent::Phase::Up && touch.zone < TouchZoneCount && touch.sample <= sample &&
                    touch.sample <= end.sample &&
                    static_cast<long double>(touch.sample) >=
                        static_cast<long double>(end.sample) - m_hold_end_release_window) {
                    const auto released_lane = lane_at(touch.sample);
                    if (released_lane && Contains(*released_lane, touch.zone / 2)) {
                        hold.last_end_contact_sample = touch.sample;
                    }
                }
            }
        }
    }

    void JudgeEngine::updateHoldActivity(const int64 sample, const uint32 slider_pressed, const uint8 side_pressed) {
        const auto can_sustain = [sample](const Hold& hold) {
            return hold.acquired && hold.points.back().state == NoteState::Pending &&
                   sample < hold.points.back().sample;
        };
        for (auto& hold : m_slider_holds) {
            hold.active = false;
            if (can_sustain(hold)) {
                const auto lane = sheets::EvaluateSliderHoldLane(hold.path, sample);
                hold.active = lane && PressedInSpan(slider_pressed, *lane);
            }
        }
        for (auto& hold : m_side_holds) {
            hold.active = can_sustain(hold) && SidePressed(side_pressed, hold.points.front().side_button);
        }
    }

    const ScoreState& JudgeEngine::score() const noexcept {
        return m_score;
    }

    bool
    JudgeEngine::acceptsBoundary(const Note& candidate, const uint8 lane, const int64 sample, const Grade grade) const {
        // Nearby overlapping notes keep their midpoint boundary even after one
        // of them has been judged. For Perfect, only the shared lanes are cut;
        // Great and Good are cut across the full width of each note.
        const long double radius = static_cast<long double>(m_good_window) * 2.0L;
        const auto first = std::lower_bound(
            m_order.begin(),
            m_order.end(),
            static_cast<long double>(candidate.sample) - radius,
            [this](const std::size_t index, const long double value) {
                return m_notes[index].sample < value;
            }
        );
        for (auto it = first; it != m_order.end(); ++it) {
            const auto& other = m_notes[*it];
            if (static_cast<long double>(other.sample) > static_cast<long double>(candidate.sample) + radius) {
                break;
            }
            if ((other.note_kind != NoteKind::Slider && other.note_kind != NoteKind::SliderHold) ||
                (other.note_kind == candidate.note_kind && other.chart_index == candidate.chart_index) ||
                other.sample == candidate.sample || !Overlaps(candidate.lane, other.lane) ||
                std::abs(static_cast<long double>(other.sample) - candidate.sample) > radius) {
                continue;
            }

            const long double midpoint =
                (static_cast<long double>(candidate.sample) + static_cast<long double>(other.sample)) / 2.0L;
            const bool crossed = candidate.sample < other.sample ? sample >= midpoint : sample < midpoint;
            if (crossed && (grade != Grade::Perfect || Contains(other.lane, lane))) {
                return false;
            }
        }
        return true;
    }

    bool JudgeEngine::acceptsSideBoundary(const Note& candidate, const int64 sample) const {
        const long double radius = static_cast<long double>(m_good_window) * 2.0L;
        const auto first = std::lower_bound(
            m_order.begin(),
            m_order.end(),
            static_cast<long double>(candidate.sample) - radius,
            [this](const std::size_t index, const long double value) {
                return m_notes[index].sample < value;
            }
        );
        for (auto it = first; it != m_order.end(); ++it) {
            const auto& other = m_notes[*it];
            if (static_cast<long double>(other.sample) > static_cast<long double>(candidate.sample) + radius) {
                break;
            }
            if ((other.note_kind != NoteKind::Side && other.note_kind != NoteKind::SideHold) ||
                (other.note_kind == candidate.note_kind && other.chart_index == candidate.chart_index) ||
                other.side_button != candidate.side_button || other.sample == candidate.sample) {
                continue;
            }
            const long double midpoint =
                (static_cast<long double>(candidate.sample) + static_cast<long double>(other.sample)) / 2.0L;
            if (candidate.sample < other.sample ? sample >= midpoint : sample < midpoint) {
                return false;
            }
        }
        return true;
    }

    Grade JudgeEngine::gradeFor(const Note& note, const int64 sample) const {
        const long double distance = std::abs(static_cast<long double>(sample) - note.sample);
        if (distance <= m_perfect_window || ((note.isXTap() || note.linked_xtap) && distance <= m_good_window)) {
            return Grade::Perfect;
        }
        if (distance <= m_great_window) {
            return Grade::Great;
        }
        return distance <= m_good_window ? Grade::Good : Grade::Miss;
    }

    JudgeEngine::HoldPoint& JudgeEngine::holdPoint(const HoldPointIndex index) {
        return (index.note_kind == NoteKind::SideHold ? m_side_holds : m_slider_holds)[index.hold].points[index.point];
    }

    void JudgeEngine::applyGrade(const Grade grade) {
        switch (grade) {
            case Grade::Perfect:
                ++m_score.perfect_count;
                ++m_score.combo;
                break;
            case Grade::Great:
                ++m_score.great_count;
                ++m_score.combo;
                break;
            case Grade::Good:
                ++m_score.good_count;
                ++m_score.combo;
                break;
            case Grade::Miss:
                ++m_score.miss_count;
                m_score.combo = 0;
                break;
        }
        m_score.max_combo = Max(m_score.max_combo, m_score.combo);
        if (m_total_combo > 0) {
            const double weighted = m_score.perfect_count * 1.1 + m_score.great_count + m_score.good_count * 0.4;
            m_score.score = static_cast<uint32>(1'000'000.0 * weighted / m_total_combo);
            const double gauge_delta = 5.0 / m_total_combo;
            m_score.clear_gauge =
                Clamp(m_score.clear_gauge + (grade == Grade::Miss ? -gauge_delta : gauge_delta), 0.0, 1.0);
        }
    }

    void JudgeEngine::record(
        Note& note,
        const Grade grade,
        const int64 sample,
        Array<JudgementEvent>& events,
        const bool linked_start
    ) {
        // Selecting the hold start must have the same result as selecting its XTap.
        if (!linked_start && grade != Grade::Miss && note.linked_xtap &&
            m_notes[*note.linked_xtap].state == NoteState::Pending) {
            record(m_notes[*note.linked_xtap], grade, sample, events);
            return;
        }
        note.state = grade == Grade::Miss ? NoteState::Missed : NoteState::Hit;
        if (note.note_kind == NoteKind::SliderHold || note.note_kind == NoteKind::SideHold) {
            auto& hold = (note.note_kind == NoteKind::SideHold ? m_side_holds : m_slider_holds)[note.chart_index];
            hold.points.front().state = note.state;
            hold.acquired = hold.acquired || grade != Grade::Miss;
        }
        applyGrade(grade);
        events.push_back(
            {
                .note_kind = note.note_kind,
                .note_index = note.chart_index,
                .point_index = note.point_index,
                .grade = grade,
                .offset_samples = sample - note.sample,
                .linked_start = linked_start,
            }
        );
        if (grade == Grade::Miss || !note.isXTap()) {
            return;
        }

        // A simultaneous overlapping XTap also starts the hold. Both count towards combo,
        // but the linked event lets audio avoid playing two start sounds.
        const auto first = std::lower_bound(
            m_order.begin(),
            m_order.end(),
            note.sample,
            [this](const std::size_t index, const int64 value) {
                return m_notes[index].sample < value;
            }
        );
        for (auto it = first; it != m_order.end() && m_notes[*it].sample == note.sample; ++it) {
            auto& start = m_notes[*it];
            const bool linked =
                start.linked_xtap && *start.linked_xtap == static_cast<std::size_t>(&note - m_notes.data());
            if (linked && start.state == NoteState::Pending) {
                record(start, grade, sample, events, true);
            }
        }
    }

    void JudgeEngine::record(HoldPoint& point, const Grade grade, const int64 sample, Array<JudgementEvent>& events) {
        point.state = grade == Grade::Miss ? NoteState::Missed : NoteState::Hit;
        auto& hold = (point.note_kind == NoteKind::SideHold ? m_side_holds : m_slider_holds)[point.hold_index];
        hold.acquired = hold.acquired || grade != Grade::Miss;
        applyGrade(grade);
        events.push_back(
            {
                .note_kind = point.note_kind,
                .note_index = point.hold_index,
                .point_index = point.point_index,
                .grade = grade,
                .offset_samples = sample - point.sample,
            }
        );
    }
}
