#pragma once

#include "MenuTimerSoundScheduler.hpp"

namespace xlair::ui::audio {
    class MenuTimerSoundPlayer {
    public:
        MenuTimerSoundPlayer() = default;
        ~MenuTimerSoundPlayer();

        MenuTimerSoundPlayer(const MenuTimerSoundPlayer&) = delete;
        MenuTimerSoundPlayer& operator=(const MenuTimerSoundPlayer&) = delete;

        void update(double remaining_seconds);
        void stop();

    private:
        MenuTimerSoundScheduler m_scheduler;
        bool m_started = false;
    };
}
