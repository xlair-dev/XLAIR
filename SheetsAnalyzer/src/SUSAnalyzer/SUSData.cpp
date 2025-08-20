#include "SUSAnalyzer/SUSData.hpp"
#include "SUSAnalyzer/internal/Utils.ipp"

namespace SheetsAnalyzer::SUSAnalyzer {
    SUSData::SUSData() {
        SUSData { Constant::DefaultSampleRate, 0 };
    }

    SUSData::SUSData(const s3d::uint64 rate, const s3d::uint64 offset) {
        sample_rate = rate;
        sample_offset = offset;
        bpm_difinitions[0] = Constant::DefaultBPM;
        beats_difinitions[0] = Constant::DefaultBeatLength;
        hispeed_difinitions[Constant::DefaultHispeedNumber] = SUSHispeedTimeline {};
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
                    break;
                case NoteType::XTapNote:
                    notes.xtap.push_back({
                        .timeline_index = note.timeline_index,
                        .sample = getSampleAt(note.time),
                        .start_lane = note.note_position.start_lane,
                        .width = note.note_position.width
                    });
                    break;
                case NoteType::FlickNote:
                    notes.flick.push_back({
                        .timeline_index = note.timeline_index,
                        .sample = getSampleAt(note.time),
                        .start_lane = note.note_position.start_lane,
                        .width = note.note_position.width
                    });
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

    s3d::uint64 SUSData::getSampleAt(const SUSRelativeNoteTime& time) {
        s3d::uint32 meas = time.measure;
        s3d::uint32 tick = time.ticks;
        s3d::uint64 sample = 0;
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

        const auto result = static_cast<s3d::uint64>(frames + 0.5); // Round to nearest integer
        return result < 0 ? 0 : result; // Ensure non-negative sample
    }
}
