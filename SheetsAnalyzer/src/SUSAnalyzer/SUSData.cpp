#include "SUSAnalyzer/SUSData.hpp"
#include "SUSAnalyzer/internal/Utils.ipp"
#include <algorithm>

namespace SheetsAnalyzer::SUSAnalyzer {
    SUSData::SUSData() {
        SUSData{ Constant::DefaultSampleRate, 0 };
    }

    SUSData::SUSData(const s3d::int64 rate, const s3d::int64 offset) {
        sample_rate = rate;
        sample_offset = offset;
        bpm_difinitions[0] = Constant::DefaultBPM;
        beats_difinitions[0] = Constant::DefaultBeatLength;
        hispeed_difinitions[Constant::DefaultHispeedNumber] = SUSHispeedTimeline{};
        hispeed_difinitions[Constant::DefaultHispeedNumber].data_index = 0;
        current_timeline = Constant::DefaultHispeedNumber;
        measure_timeline = Constant::DefaultHispeedNumber;
    }

    void SUSData::convertToSheetData() {
        // timelines
        timelines.resize(hispeed_difinitions.size());
        for (const auto& [index, hispeed_difinition] : hispeed_difinitions) {
            auto& timeline = timelines[hispeed_difinition.data_index];
            timeline.hispeed_data.reserve(hispeed_difinition.hispeed_data.size());

            for (const auto& hispeed : hispeed_difinition.hispeed_data) {
                timeline.hispeed_data.push_back({
                    .sample = getSampleAt(hispeed.time),
                    .speed = hispeed.speed
                });
            }
        }

        // bpm changes
        bpm_changes.resize(bpm_notes.size());
        for (const auto& bpm_note : bpm_notes) {
            bpm_changes.push_back({
                .sample = getSampleAt(bpm_note.time),
                .bpm = bpm_difinitions[bpm_note.definition_index]
            });
        }

        s3d::Array<HoldNoteData> holdnote_channel(36);
        s3d::Array<SUSRelativeNoteTime> holdnote_last_judge(36);

        // raw notes
        for (const auto& note : raw_notes) {
            switch (note.type) {
                case NoteType::TapNote:
                    notes.tap.push_back({
                        .timeline_index = note.timeline_index,
                        .sample = getSampleAt(note.time),
                        .start_lane = note.note_position.start_lane,
                        .width = note.note_position.width
                    });
                    total_combo++;
                    break;
                case NoteType::XTapNote:
                    notes.xtap.push_back({
                        .timeline_index = note.timeline_index,
                        .sample = getSampleAt(note.time),
                        .start_lane = note.note_position.start_lane,
                        .width = note.note_position.width
                    });
                    total_combo++;
                    break;
                case NoteType::FlickNote:
                    notes.flick.push_back({
                        .timeline_index = note.timeline_index,
                        .sample = getSampleAt(note.time),
                        .start_lane = note.note_position.start_lane,
                        .width = note.note_position.width
                    });
                    total_combo++;
                    break;
                case NoteType::HoldStartNote:
                    holdnote_channel[note.extra].clear();
                    holdnote_channel[note.extra].notes.push_back({
                        .timeline_index = note.timeline_index,
                        .sample = getSampleAt(note.time),
                        .start_lane = note.note_position.start_lane,
                        .width = note.note_position.width,
                        .type = HoldNoteType::Start,
                    });
                    holdnote_last_judge[note.extra] = note.time;
                    break;
                case NoteType::HoldEndNote:
                    holdnote_channel[note.extra].notes.push_back({
                        .timeline_index = note.timeline_index,
                        .sample = getSampleAt(note.time),
                        .start_lane = note.note_position.start_lane,
                        .width = note.note_position.width,
                        .type = HoldNoteType::End,
                    });
                    constructHoldJudge(holdnote_channel[note.extra], holdnote_last_judge[note.extra]);
                    holdnote_channel[note.extra].judge.push_back({
                        .timeline_index = note.timeline_index,
                        .sample = getSampleAt(note.time),
                        .start_lane = note.note_position.start_lane,
                        .width = note.note_position.width,
                        .type = HoldNoteType::End,
                    });

                    total_combo += holdnote_channel[note.extra].judge.size();
                    notes.hold.push_back(std::move(holdnote_channel[note.extra]));
                    holdnote_last_judge[note.extra] = SUSRelativeNoteTime{};
                    holdnote_channel[note.extra] = HoldNoteData{};
                    break;
                case NoteType::HoldControlPoint:
                    // TODO: implement
                    break;
                case NoteType::HoldMidNote:
                    holdnote_channel[note.extra].notes.push_back({
                        .timeline_index = note.timeline_index,
                        .sample = getSampleAt(note.time),
                        .start_lane = note.note_position.start_lane,
                        .width = note.note_position.width,
                        .type = HoldNoteType::Mid,
                    });
                    constructHoldJudge(holdnote_channel[note.extra], holdnote_last_judge[note.extra]);
                    holdnote_last_judge[note.extra] = note.time;
                    holdnote_channel[note.extra].judge.push_back({
                        .timeline_index = note.timeline_index,
                        .sample = getSampleAt(note.time),
                        .start_lane = note.note_position.start_lane,
                        .width = note.note_position.width,
                        .type = HoldNoteType::Mid,
                    });
                    break;
                case NoteType::HoldMidUnvisibleNote:
                    holdnote_channel[note.extra].notes.push_back({
                        .timeline_index = note.timeline_index,
                        .sample = getSampleAt(note.time),
                        .start_lane = note.note_position.start_lane,
                        .width = note.note_position.width,
                        .type = HoldNoteType::MidUnvisible,
                    });
                    constructHoldJudge(holdnote_channel[note.extra], holdnote_last_judge[note.extra]);
                    break;
                default:
                    // Ignore other types
                    break;
            }
        }

        valid = true;
    }

    float SUSData::getBeatsAt(const s3d::uint32 meas) const {
        const auto it = beats_difinitions.upper_bound(meas);
        if (it == beats_difinitions.begin()) {
            // Default beat length if no definition found
            return Constant::DefaultBeatLength;
        }
        // Return the last defined beat length before the given measure
        return std::prev(it)->second;
    }

    s3d::int64 SUSData::getSampleAt(const SUSRelativeNoteTime& time) {
        s3d::uint32 meas = time.measure;
        s3d::uint32 tick = time.ticks;
        double frames = 0.0;
        double last_bpm = Constant::DefaultBPM;
        const double inv_tpb = 1.0 / static_cast<double>(ticks_per_beat);

        // extended ticks
        while (tick >= static_cast<s3d::uint32>(getBeatsAt(meas) * ticks_per_beat)) {
            tick -= static_cast<s3d::uint32>(getBeatsAt(meas) * ticks_per_beat);
            meas++;
        }

        for (s3d::uint32 i = 0; i <= meas; i++) {
            const double beats = static_cast<double>(getBeatsAt(i));
            s3d::uint32 last_change_tick = 0;

            for (const auto& bpm_note : bpm_notes) {
                const auto timing = bpm_note.time;
                const auto definition_index = bpm_note.definition_index;
                if (timing.measure != i) {
                    continue;
                }
                if (i == meas and timing.ticks >= tick) {
                    // If we are at the current measure and the bpm change is after the current tick
                    break;
                }

                const double samples_per_beat = sample_rate * 60.0 / last_bpm;
                const double delta_beats = static_cast<double>(timing.ticks - last_change_tick) * inv_tpb;

                frames += delta_beats * samples_per_beat;

                last_change_tick = timing.ticks;
                last_bpm = bpm_difinitions[definition_index];
            }

            const double samples_per_beat = sample_rate * 60.0 / last_bpm;
            if (i == meas) {
                // If we are at the current measure, calculate the remaining ticks
                const double delta_beats = static_cast<double>(tick - last_change_tick) * inv_tpb;
                frames += delta_beats * samples_per_beat;
            } else {
                // Otherwise, add the full beats
                const double ticks_in_measure = static_cast<double>(beats * ticks_per_beat);
                const double delta_beats = (ticks_in_measure - last_change_tick) * inv_tpb;
                frames += delta_beats * samples_per_beat;
            }
        }

        frames += sample_offset;

        const auto result = static_cast<s3d::int64>(frames + 0.5); // Round to nearest integer
        return result < 0 ? 0 : result; // Ensure non-negative sample
    }

    void SUSData::constructHoldJudge(HoldNoteData& data, SUSRelativeNoteTime& last_time) {
        // data.judge を作る
        // last_time -> data.notes.back().sample まで分割
        if (data.notes.size() < 2) {
            return;
        }

        const auto& a = data.notes[data.notes.size() - 2];
        const auto& b = data.notes[data.notes.size() - 1];

        const auto diff = static_cast<double>(b.sample - a.sample);
        if (diff <= 0.0) return;

        const double leftA = static_cast<double>(a.start_lane);
        const double rightA = static_cast<double>(a.start_lane + a.width);
        const double leftB = static_cast<double>(b.start_lane);
        const double rightB = static_cast<double>(b.start_lane + b.width);

        const auto end_sample = data.notes.back().sample;
        s3d::int64 cur_sample = getSampleAt(last_time);
        while (cur_sample < end_sample) {
            {
                const double t = (static_cast<double>(cur_sample) - static_cast<double>(a.sample)) / diff;
                const double u = s3d::Clamp(t, 0.0, 1.0);

                // 端を補間
                const double leftF = s3d::Math::Lerp(leftA, leftB, u);
                const double rightF = s3d::Math::Lerp(rightA, rightB, u);

                // 端を安定丸め（微小補正で境界のチラつきを防ぐ）
                s3d::int32 left = static_cast<s3d::int32>(s3d::Math::Floor(leftF + 1e-9));
                s3d::int32 right = static_cast<s3d::int32>(s3d::Math::Ceil(rightF - 1e-9));

                // TODO: should I clamp?
                const s3d::uint8 start_lane = static_cast<s3d::uint8>(left);
                const s3d::uint8 width = static_cast<s3d::uint8>(right - left);

                data.judge.push_back({
                    .timeline_index = 0, // 判定用なので何でもよい
                    .sample = cur_sample,
                    .start_lane = start_lane,
                    .width = width,
                    .type = HoldNoteType::MidUnvisible, // 判定用なので何でもよい
                });
            }

            const auto bpm = std::prev(
                std::ranges::upper_bound(bpm_changes, cur_sample, {}, &BPMData::sample)
            )->bpm; // 必ず sample=0 の 1 要素はあるので begin は来ないはず
            const s3d::uint32 div = (bpm < 120.0 ? 4u : (bpm < 240.0 ? 2u : 1u));
            const s3d::uint32 dt = ticks_per_beat / div; // TODO: div で割り切れない場合も考慮する？
            last_time.ticks += dt;
            cur_sample = getSampleAt(last_time);
        }
    }
}
