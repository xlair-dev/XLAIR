#include "GameplaySoundPlayer.hpp"

#include "AudioBus.hpp"

#include <algorithm>

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
        m_events.clear();
        m_cursor = 0;
        for (const auto& note : chart.slider_notes) {
            Sound sound = Sound::Tap;
            switch (note.kind) {
                case sheets::SliderNoteKind::Tap:
                    break;
                case sheets::SliderNoteKind::XTap:
                    sound = Sound::XTap;
                    break;
                case sheets::SliderNoteKind::Flick:
                    sound = Sound::Flick;
                    break;
            }
            m_events.push_back({ note.sample, sound });
        }
        for (const auto& note : chart.side_notes) {
            m_events.push_back({ note.sample, note.kind == sheets::SideNoteKind::XTap ? Sound::XTap : Sound::SideTap });
        }
        for (const auto& hold : chart.slider_holds) {
            if (hold.points.isEmpty()) {
                continue;
            }
            const auto& start = hold.points.front();
            const bool starts_with_xtap =
                std::any_of(chart.slider_notes.begin(), chart.slider_notes.end(), [&](const auto& note) {
                    return note.kind == sheets::SliderNoteKind::XTap && note.sample == start.sample &&
                           note.lane.start == start.lane.start && note.lane.width == start.lane.width;
                });
            if (!starts_with_xtap) {
                m_events.push_back({ start.sample, Sound::SlideTap });
            }
            // Generated judgement ticks and curve subdivisions must not produce checkpoint sounds.
            for (const auto& point : hold.points) {
                if (point.kind == sheets::SliderHoldPointKind::Visible ||
                    point.kind == sheets::SliderHoldPointKind::End) {
                    m_events.push_back({ point.sample, Sound::Checkpoint });
                }
            }
            m_events.push_back({ start.sample, Sound::SlideHold, 1 });
            m_events.push_back({ hold.points.back().sample, Sound::SlideHold, -1 });
        }
        for (const auto& hold : chart.side_holds) {
            if (hold.points.isEmpty()) {
                continue;
            }
            const int64 start = hold.points.front().sample;
            const bool starts_with_xtap =
                std::any_of(chart.side_notes.begin(), chart.side_notes.end(), [&](const auto& note) {
                    return note.kind == sheets::SideNoteKind::XTap && note.sample == start &&
                           note.button == hold.button;
                });
            if (!starts_with_xtap) {
                m_events.push_back({ start, Sound::SideTap });
            }
            for (const auto& point : hold.points) {
                if (point.kind == sheets::SideHoldPointKind::Relay || point.kind == sheets::SideHoldPointKind::End) {
                    m_events.push_back({ point.sample, Sound::Checkpoint });
                }
            }
            m_events.push_back({ start, Sound::SideHold, 1 });
            m_events.push_back({ hold.points.back().sample, Sound::SideHold, -1 });
        }
        m_events.stable_sort_by([](const auto& left, const auto& right) {
            return left.sample < right.sample;
        });
        // Music playback begins at sample zero; recover holds spanning zero without replaying past hits.
        while (m_cursor < m_events.size() && m_events[m_cursor].sample < 0) {
            const auto& event = m_events[m_cursor++];
            if (event.change != 0) {
                m_active_holds[static_cast<std::size_t>(event.sound)] += event.change;
            }
        }
    }

    void GameplaySoundPlayer::update(const int64 sample) {
        while (m_cursor < m_events.size() && m_events[m_cursor].sample <= sample) {
            const auto& event = m_events[m_cursor++];
            if (event.change == 0) {
                m_sounds[static_cast<std::size_t>(event.sound)].playOneShot(GameplaySound, JudgmentVolume);
            } else {
                m_active_holds[static_cast<std::size_t>(event.sound)] += event.change;
            }
        }
        for (const auto kind : { Sound::SideHold, Sound::SlideHold }) {
            const auto index = static_cast<std::size_t>(kind);
            const auto& sound = m_sounds[index];
            if (m_active_holds[index] > 0) {
                if (!sound.isPlaying()) {
                    sound.play(GameplaySound);
                }
            } else {
                sound.stop();
            }
        }
    }

    void GameplaySoundPlayer::finish(const int64 sample) {
        update(sample);
        for (const auto kind : { Sound::SideHold, Sound::SlideHold }) {
            m_sounds[static_cast<std::size_t>(kind)].stop();
        }
        m_active_holds.fill(0);
        m_cursor = m_events.size();
    }

    void GameplaySoundPlayer::stop() {
        for (const auto& sound : m_sounds) {
            sound.stop();
            sound.stopAllShots();
        }
        m_active_holds.fill(0);
        m_cursor = m_events.size();
    }
}
