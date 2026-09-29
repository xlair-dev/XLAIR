#pragma once

#include "Common.hpp"

namespace xlair::ui::audio {
    class MusicPreviewPlayer {
    public:
        MusicPreviewPlayer() = default;
        ~MusicPreviewPlayer();

        MusicPreviewPlayer(const MusicPreviewPlayer&) = delete;
        MusicPreviewPlayer& operator=(const MusicPreviewPlayer&) = delete;

        void update(FilePathView path, double start_seconds);
        void stop(const Duration& fade_out = SecondsF{ 0.0 });

    private:
        void startLoading();
        void finishLoading();
        void updatePlayback();

        Audio m_audio;
        Stopwatch m_stopwatch;
        FilePath m_path;
        FilePath m_loading_path;
        double m_start_seconds = 0.0;
        AsyncTask<Audio> m_load_task;
        int32 m_preview_duration_ms = 0;
        int32 m_fade_duration_ms = 0;
        bool m_done = true;
        bool m_changed = false;
        bool m_valid = false;
        bool m_enabled = false;
    };
}
