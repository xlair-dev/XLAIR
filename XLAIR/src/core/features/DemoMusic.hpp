#pragma once
#include "Common.hpp"

namespace core::features {
    class DemoMusic {
    public:
        DemoMusic() = default;
        DemoMusic(const FilePath& path, double demo_start);
        ~DemoMusic();

        bool update(const FilePath& path, double demo_start);
        void autoPlayAndStop();
        bool done() const;
        bool stop(const Duration& fadeout_duration = SecondsF(0.0)) const;

    private:
        Audio m_audio;
        Stopwatch m_stopwatch;
        FilePath m_path;
        double m_demo_start = 0;

        AsyncTask<Audio> m_task;
        bool m_done = false;
        bool m_changed = false;
        bool m_valid = false;
    };
}
