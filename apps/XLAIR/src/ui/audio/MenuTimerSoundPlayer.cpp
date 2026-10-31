#include "MenuTimerSoundPlayer.hpp"

#include "AudioBus.hpp"
#include "SoundEffect.hpp"
#include "ui/assets/Assets.hpp"

namespace xlair::ui::audio {
    MenuTimerSoundPlayer::~MenuTimerSoundPlayer() {
        stop();
    }

    void MenuTimerSoundPlayer::update(const double remaining_seconds) {
        if (!std::isfinite(remaining_seconds) || remaining_seconds <= 0.0) {
            stop();
            return;
        }

        const auto cue = m_scheduler.update(remaining_seconds);
        switch (cue.kind) {
            case MenuTimerSoundScheduler::Kind::Normal:
                PlaySoundEffect(SoundEffect::TimerNormal);
                m_started = true;
                break;
            case MenuTimerSoundScheduler::Kind::Special: {
                const AudioAsset sound{ assets::sound::TimerSpecial };
                // The asset contains all three final counts. Seek past any missed time instead of stacking cues.
                sound.stop();
                sound.seekTime(cue.offset_seconds);
                sound.play(UISound);
                m_started = true;
                break;
            }
            case MenuTimerSoundScheduler::Kind::None:
                break;
        }
    }

    void MenuTimerSoundPlayer::stop() {
        if (m_started) {
            AudioAsset{ assets::sound::TimerNormal }.stopAllShots();
            AudioAsset{ assets::sound::TimerSpecial }.stop();
            m_started = false;
        }
        m_scheduler.reset();
    }
}
