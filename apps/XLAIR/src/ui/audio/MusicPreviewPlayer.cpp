#include "MusicPreviewPlayer.hpp"

#include <cmath>
#include <exception>
#include <utility>

namespace xlair::ui::audio {
    namespace {
        constexpr Duration SelectionFadeOut{ 0.4 };
        constexpr int32 FadeDurationMs = 500;
        constexpr int32 PreviewDurationMs = 29'800;
        constexpr int32 RestartDelayMs = 200;

        [[nodiscard]]
        Audio LoadStream(const FilePath& path) {
            return Audio{ Audio::Stream, path };
        }
    }

    MusicPreviewPlayer::~MusicPreviewPlayer() {
        stop();
        if (m_audio) {
            m_audio.release();
        }
    }

    void MusicPreviewPlayer::update(const FilePathView path, const double start_seconds) {
        if (path.isEmpty()) {
            stop(SelectionFadeOut);
            finishLoading();
            return;
        }

        m_enabled = true;
        const double safe_start = std::isfinite(start_seconds) ? Max(0.0, start_seconds) : 0.0;
        if (!m_valid) {
            m_path = path;
            m_start_seconds = safe_start;
            m_valid = true;
            startLoading();
        } else if (m_path != path) {
            m_changed = true;
            m_path = path;
            m_start_seconds = safe_start;
            if (m_audio) {
                m_audio.stop(SelectionFadeOut);
                m_stopwatch.reset();
            }
        } else if (m_start_seconds != safe_start) {
            m_start_seconds = safe_start;
            m_stopwatch.reset();
            if (m_audio && m_audio.isPlaying()) {
                m_audio.stop(SelectionFadeOut);
            }
        }

        finishLoading();

        if (m_changed && m_done) {
            m_changed = false;
            startLoading();
        }

        updatePlayback();
    }

    void MusicPreviewPlayer::stop(const Duration& fade_out) {
        if (!m_enabled) {
            return;
        }
        m_enabled = false;
        m_stopwatch.reset();
        if (m_audio) {
            m_audio.stop(fade_out);
        }
    }

    void MusicPreviewPlayer::startLoading() {
        m_loading_path = m_path;
        m_load_task = Async(LoadStream, m_loading_path);
        m_done = false;
    }

    void MusicPreviewPlayer::finishLoading() {
        if (m_done || !m_load_task.isReady()) {
            return;
        }

        Audio loaded;
        bool failed = false;
        try {
            loaded = m_load_task.get();
            failed = !loaded;
        } catch (const Error&) {
            failed = true;
        } catch (const std::exception&) {
            failed = true;
        }

        if (!m_changed) {
            if (m_audio) {
                m_audio.stop();
                m_audio.release();
            }
            m_audio = std::move(loaded);
            if (failed) {
                Logger << U"[MusicPreview] Failed to load '{}'."_fmt(m_loading_path);
            }
        }
        m_done = true;
    }

    void MusicPreviewPlayer::updatePlayback() {
        if (!m_enabled || !m_done || !m_audio) {
            return;
        }

        if (!m_stopwatch.isRunning()) {
            if (m_audio.isPlaying()) {
                return;
            }

            const double length_seconds = m_audio.lengthSec();
            if (!std::isfinite(length_seconds) || length_seconds <= 0.0) {
                Logger << U"[MusicPreview] The audio has no playable duration: '{}'."_fmt(m_path);
                m_enabled = false;
                return;
            }

            const double start_seconds = m_start_seconds < length_seconds ? m_start_seconds : 0.0;
            const double available_seconds = length_seconds - start_seconds;
            const double preview_seconds = Min(PreviewDurationMs / 1'000.0, available_seconds);
            m_preview_duration_ms = Max(1, static_cast<int32>(std::round(preview_seconds * 1'000.0)));
            m_fade_duration_ms = Min(FadeDurationMs, m_preview_duration_ms / 2);

            m_stopwatch.restart();
            m_audio.setVolume(0.0);
            m_audio.seekTime(start_seconds);
            m_audio.play();
        }

        const int32 elapsed_ms = m_stopwatch.ms();
        if (m_fade_duration_ms > 0 && elapsed_ms < m_fade_duration_ms) {
            m_audio.setVolume(static_cast<double>(elapsed_ms) / m_fade_duration_ms);
        } else if (elapsed_ms < m_preview_duration_ms - m_fade_duration_ms) {
            m_audio.setVolume(1.0);
        } else if (m_fade_duration_ms > 0 && elapsed_ms < m_preview_duration_ms) {
            m_audio.setVolume(static_cast<double>(m_preview_duration_ms - elapsed_ms) / m_fade_duration_ms);
        } else if (elapsed_ms > m_preview_duration_ms + RestartDelayMs) {
            m_stopwatch.reset();
            m_audio.stop();
        }
    }
}
