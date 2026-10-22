#include "GameplaySoundPlayer.hpp"

#include "AudioBus.hpp"

namespace xlair::ui::audio {
    namespace {
        constexpr double JudgmentVolume = 0.55;
    }

    GameplaySoundPlayer::GameplaySoundPlayer() {
        constexpr std::array<StringView, static_cast<std::size_t>(GameplaySoundKind::Count)> filenames{
            U"tap", U"slide-tap", U"x-tap", U"flick", U"side-tap", U"checkpoint", U"side-hold", U"slide-hold",
        };
        for (std::size_t i = 0; i < m_sounds.size(); ++i) {
            const FilePath path = U"ui/sounds/judgments/{}.mp3"_fmt(filenames[i]);
            m_sounds[i] = Audio{ Resource(path) };
            if (!m_sounds[i]) {
                throw Error{ U"Failed to load gameplay sound: " + path };
            }
        }
        for (const auto kind : { GameplaySoundKind::SideHold, GameplaySoundKind::SlideHold }) {
            const auto& sound = m_sounds[static_cast<std::size_t>(kind)];
            sound.setVolume(JudgmentVolume);
            sound.setLoop(true);
        }
    }

    GameplaySoundPlayer::~GameplaySoundPlayer() {
        stop();
    }

    void GameplaySoundPlayer::start(const sheets::Chart& chart) {
        stop();
        m_scheduler.start(chart);
    }

    void GameplaySoundPlayer::update(
        const int64 music_sample,
        const core::judgement::JudgeEngine& judge,
        const std::span<const core::judgement::JudgementEvent> events
    ) {
        const auto frame = m_scheduler.update(music_sample, judge, events);
        for (const auto sound : frame.one_shots) {
            m_sounds[static_cast<std::size_t>(sound)].playOneShot(GameplaySound, JudgmentVolume);
        }
        syncLoop(GameplaySoundKind::SlideHold, frame.slider_hold_active);
        syncLoop(GameplaySoundKind::SideHold, frame.side_hold_active);
    }

    void GameplaySoundPlayer::syncLoop(const GameplaySoundKind kind, const bool active) {
        const auto& sound = m_sounds[static_cast<std::size_t>(kind)];
        if (active) {
            if (!sound.isPlaying()) {
                sound.play(GameplaySound);
            }
        } else if (sound.isPlaying()) {
            sound.stop();
        }
    }

    void GameplaySoundPlayer::finish() {
        m_scheduler.clear();
        syncLoop(GameplaySoundKind::SideHold, false);
        syncLoop(GameplaySoundKind::SlideHold, false);
    }

    void GameplaySoundPlayer::stop() {
        m_scheduler.clear();
        for (const auto& sound : m_sounds) {
            sound.stop();
            sound.stopAllShots();
        }
    }
}
