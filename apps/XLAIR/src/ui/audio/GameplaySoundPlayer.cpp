#include "GameplaySoundPlayer.hpp"

#include "AudioBus.hpp"

#include <algorithm>
#include <utility>

namespace xlair::ui::audio {
    namespace {
        constexpr double JudgmentVolume = 0.55;
    }

    GameplaySoundPlayer::GameplaySoundPlayer() {
        constexpr std::array<StringView, static_cast<std::size_t>(Sound::Count)> filenames{
            U"tap", U"slide-tap", U"x-tap", U"flick", U"side-tap", U"checkpoint", U"side-hold", U"slide-hold",
        };
        for (std::size_t i = 0; i < m_sounds.size(); ++i) {
            const FilePath path = U"ui/sounds/judgments/{}.mp3"_fmt(filenames[i]);
            m_sounds[i] = Audio{ Resource(path) };
            if (!m_sounds[i]) {
                throw Error{ U"Failed to load gameplay sound: " + path };
            }
            m_sounds[i].setVolume(JudgmentVolume);
        }
        m_sounds[static_cast<std::size_t>(Sound::SideHold)].setLoop(true);
        m_sounds[static_cast<std::size_t>(Sound::SlideHold)].setLoop(true);
    }

    GameplaySoundPlayer::~GameplaySoundPlayer() {
        stop();
    }

    void GameplaySoundPlayer::start(const sheets::Chart& chart) {
        stop();
        m_slider_notes.clear();
        m_side_notes.clear();
        m_slider_holds.clear();
        m_side_holds.clear();
        for (const auto& note : chart.slider_notes) {
            switch (note.kind) {
                case sheets::SliderNoteKind::Tap:
                    m_slider_notes.push_back(Sound::Tap);
                    break;
                case sheets::SliderNoteKind::XTap:
                    m_slider_notes.push_back(Sound::XTap);
                    break;
                case sheets::SliderNoteKind::Flick:
                    m_slider_notes.push_back(Sound::Flick);
                    break;
            }
        }
        for (const auto& note : chart.side_notes) {
            m_side_notes.push_back(note.kind == sheets::SideNoteKind::XTap ? Sound::XTap : Sound::SideTap);
        }
        for (const auto& hold : chart.slider_holds) {
            Array<Optional<Sound>> sounds(hold.judge_points.size());
            if (!hold.judge_points.isEmpty()) {
                sounds.front() = Sound::SlideTap;
                for (std::size_t index = 1; index < sounds.size(); ++index) {
                    if (std::any_of(hold.points.begin(), hold.points.end(), [&](const auto& point) {
                            return point.sample == hold.judge_points[index].sample &&
                                   (point.kind == sheets::SliderHoldPointKind::Visible ||
                                    point.kind == sheets::SliderHoldPointKind::End);
                        })) {
                        sounds[index] = Sound::Checkpoint;
                    }
                }
            }
            m_slider_holds.push_back(std::move(sounds));
        }
        for (const auto& hold : chart.side_holds) {
            Array<Optional<Sound>> sounds(hold.judge_samples.size());
            if (!hold.judge_samples.isEmpty()) {
                sounds.front() = Sound::SideTap;
                for (std::size_t index = 1; index < sounds.size(); ++index) {
                    if (std::any_of(hold.points.begin(), hold.points.end(), [&](const auto& point) {
                            return point.sample == hold.judge_samples[index] &&
                                   (point.kind == sheets::SideHoldPointKind::Relay ||
                                    point.kind == sheets::SideHoldPointKind::End);
                        })) {
                        sounds[index] = Sound::Checkpoint;
                    }
                }
            }
            m_side_holds.push_back(std::move(sounds));
        }
    }

    void GameplaySoundPlayer::update(
        const core::judgement::JudgeEngine& judge,
        const std::span<const core::judgement::JudgementEvent> events
    ) {
        for (const auto& event : events) {
            if (event.grade == core::judgement::Grade::Miss || event.linked_start) {
                continue;
            }
            Optional<Sound> sound;
            switch (event.note_kind) {
                case core::judgement::NoteKind::Slider:
                    sound = m_slider_notes.at(event.note_index);
                    break;
                case core::judgement::NoteKind::Side:
                    sound = m_side_notes.at(event.note_index);
                    break;
                case core::judgement::NoteKind::SliderHold:
                    sound = m_slider_holds.at(event.note_index).at(event.point_index);
                    break;
                case core::judgement::NoteKind::SideHold:
                    sound = m_side_holds.at(event.note_index).at(event.point_index);
                    break;
            }
            if (sound) {
                m_sounds[static_cast<std::size_t>(*sound)].playOneShot(GameplaySound, JudgmentVolume);
            }
        }
        bool slide_active = false;
        for (std::size_t index = 0; index < m_slider_holds.size(); ++index) {
            slide_active = slide_active || judge.sliderHoldActive(index);
        }
        bool side_active = false;
        for (std::size_t index = 0; index < m_side_holds.size(); ++index) {
            side_active = side_active || judge.sideHoldActive(index);
        }
        syncLoop(Sound::SlideHold, slide_active);
        syncLoop(Sound::SideHold, side_active);
    }

    void GameplaySoundPlayer::syncLoop(const Sound kind, const bool active) {
        const auto& sound = m_sounds[static_cast<std::size_t>(kind)];
        if (active) {
            if (!sound.isPlaying()) {
                sound.play(GameplaySound);
            }
        } else {
            sound.stop();
        }
    }

    void GameplaySoundPlayer::finish() {
        syncLoop(Sound::SideHold, false);
        syncLoop(Sound::SlideHold, false);
    }

    void GameplaySoundPlayer::stop() {
        for (const auto& sound : m_sounds) {
            sound.stop();
            sound.stopAllShots();
        }
    }
}
