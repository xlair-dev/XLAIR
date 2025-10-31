#include "DemoMusic.hpp"

namespace core::features {
    namespace {
        Audio CreateAudio(const FilePath& path) {
            return Audio(Audio::Stream, path);
        }
    }

    DemoMusic::DemoMusic(const FilePath& path, double demo_start)
        : m_path(path)
        , m_demo_start(demo_start)
        , m_valid(true) {
        m_task = Async(CreateAudio, path);
    }

    DemoMusic::~DemoMusic() {
        if (m_audio) {
            m_audio.stop();
            m_audio.release();
        }
    }

    bool DemoMusic::update(const FilePath& path, double demo_start) {
        if (not m_valid) {
            m_path = path;
            m_demo_start = demo_start;
            m_task = Async(CreateAudio, path);
            m_valid = true;
            return true;
        }
        
        if (m_path != path) {
            m_changed = true;
            m_path = path;
            m_demo_start = demo_start;
            if (m_audio) {
                m_audio.stop(0.4s);
                m_stopwatch.reset();
            }
        }

        if (!m_done && m_task.isReady()) {
            if (!m_changed) {
                if (m_audio) {
                    m_audio.stop();
                    m_audio.release();
                }
                m_audio = m_task.get();
            }
            m_done = true;
        }

        if (m_changed) {
            if (m_done) {
                m_task = Async(CreateAudio, m_path);
                m_changed = m_done = false;
            }
        }
        return m_changed;
    }

    void DemoMusic::autoPlayAndStop() {
        if (not m_valid) {
            return;
        }

        if (m_done && !m_audio.isPlaying()) {
            if (m_audio) {
                m_stopwatch.restart();
                m_audio.setVolume(0.0);
                m_audio.seekTime(m_demo_start);
                m_audio.play();
            }
        }

        if (m_stopwatch.isRunning() && m_audio) {
            constexpr int32 fadeInOutMs = 500;
            constexpr int32 length = 29800;
            constexpr int32 span = 200;

            int32 elapsedMs = m_stopwatch.ms();

            if (elapsedMs < fadeInOutMs) {
                m_audio.setVolume((double)elapsedMs / fadeInOutMs);
            } else if (elapsedMs < length - fadeInOutMs) {
                m_audio.setVolume(1.0);
            } else if (elapsedMs < length) {
                m_audio.setVolume((double)(length - elapsedMs) / fadeInOutMs);
            } else if (elapsedMs > length + span) {
                m_stopwatch.reset();
                m_audio.stop();
            }
        }
    }

    bool DemoMusic::done() const {
        return m_done;
    }

    bool DemoMusic::stop(const Duration& fadeout_duration) const {
        if (m_done && m_audio) {
            m_audio.stop(fadeout_duration);
            return true;
        }
        return false;
    }
}
