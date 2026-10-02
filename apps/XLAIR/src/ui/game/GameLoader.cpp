#include "GameLoader.hpp"

#include <algorithm>
#include <utility>

namespace xlair::ui::game {
    namespace {
        [[nodiscard]]
        Audio LoadAudio(const FilePath& path) {
            return Audio{ Audio::Stream, path };
        }
    }

    GameLoader::~GameLoader() {
        cancel();
        if (m_audio_task.isValid()) {
            m_audio_task.wait();
        }
        if (m_chart_task.isValid()) {
            m_chart_task.wait();
        }
        for (auto& task : m_retired_audio_tasks) {
            task.wait();
        }
        for (auto& task : m_retired_chart_tasks) {
            task.wait();
        }
    }

    void GameLoader::start(const GameSelection& selection) {
        cancel();
        m_error.clear();
        m_diagnostics.clear();
        m_chart_path = selection.difficulty.src;
        m_offset_seconds = selection.music_offset_seconds;

        if (selection.music_path.isEmpty()) {
            fail(U"The selected music does not specify an audio file.");
            return;
        }
        if (m_chart_path.isEmpty()) {
            fail(U"The selected difficulty does not specify a chart file.");
            return;
        }

        const FilePath audio_path = selection.music_path;
        m_audio_task = AsyncTask<Audio>{ [audio_path]() {
            return LoadAudio(audio_path);
        } };
        m_state = State::LoadingAudio;
    }

    void GameLoader::update() {
        reapRetiredTasks();
        switch (m_state) {
            case State::LoadingAudio:
                updateAudio();
                break;
            case State::LoadingChart:
                updateChart();
                break;
            default:
                break;
        }
    }

    void GameLoader::cancel() {
        // The workers capture file paths by value. Retire them instead of waiting on a scene transition.
        if (m_state == State::LoadingAudio && m_audio_task.isValid()) {
            m_retired_audio_tasks.push_back(std::move(m_audio_task));
        }
        if (m_state == State::LoadingChart && m_chart_task.isValid()) {
            m_retired_chart_tasks.push_back(std::move(m_chart_task));
        }

        if (m_audio) {
            m_audio.stop();
            m_audio.release();
        }
        m_projection.reset();
        m_chart.reset();
        m_diagnostics.clear();
        m_error.clear();
        m_state = State::Idle;
    }

    GameLoader::State GameLoader::state() const noexcept {
        return m_state;
    }

    const String& GameLoader::error() const noexcept {
        return m_error;
    }

    const Array<sheets::Diagnostic>& GameLoader::diagnostics() const noexcept {
        return m_diagnostics;
    }

    const Audio& GameLoader::audio() const noexcept {
        return m_audio;
    }

    const Optional<sheets::Chart>& GameLoader::chart() const noexcept {
        return m_chart;
    }

    const Optional<playfield::ChartProjection>& GameLoader::projection() const noexcept {
        return m_projection;
    }

    void GameLoader::updateAudio() {
        if (!m_audio_task.isReady()) {
            return;
        }

        try {
            m_audio = m_audio_task.get();
        } catch (...) {
            fail(U"An unexpected error occurred while loading the audio file.");
            return;
        }
        if (!m_audio || m_audio.sampleRate() <= 0) {
            fail(U"Failed to load the selected audio file.");
            return;
        }

        const FilePath chart_path = m_chart_path;
        const int64 sample_rate = m_audio.sampleRate();
        const double offset_seconds = m_offset_seconds;
        m_chart_task = AsyncTask<sheets::Result<sheets::Chart>>{ [chart_path, sample_rate, offset_seconds]() {
            return sheets::LoadChart(
                chart_path,
                {
                    .sample_rate = sample_rate,
                    .offset_seconds = offset_seconds,
                }
            );
        } };
        m_state = State::LoadingChart;
    }

    void GameLoader::updateChart() {
        if (!m_chart_task.isReady()) {
            return;
        }

        try {
            auto result = m_chart_task.get();
            m_diagnostics = std::move(result.diagnostics);
            for (const auto& diagnostic : m_diagnostics) {
                if (diagnostic.path.isEmpty()) {
                    Logger << U"[Game] " + diagnostic.message;
                } else {
                    Logger << U"[Game] {}: {}"_fmt(diagnostic.path, diagnostic.message);
                }
            }
            if (!result) {
                const auto error =
                    std::find_if(m_diagnostics.begin(), m_diagnostics.end(), [](const sheets::Diagnostic& diagnostic) {
                        return diagnostic.severity == sheets::DiagnosticSeverity::Error;
                    });
                fail(error == m_diagnostics.end() ? U"Failed to load the selected chart." : error->message);
                return;
            }
            m_chart = std::move(*result);
            m_projection.emplace(*m_chart);
            m_state = State::Ready;
            Logger << U"[Game] Loaded chart: {} notes at {} Hz."_fmt(m_chart->total_combo, m_chart->sample_rate);
        } catch (...) {
            fail(U"An unexpected error occurred while loading the chart.");
        }
    }

    void GameLoader::reapRetiredTasks() {
        for (std::size_t i = 0; i < m_retired_audio_tasks.size();) {
            if (!m_retired_audio_tasks[i].isReady()) {
                ++i;
                continue;
            }
            try {
                (void)m_retired_audio_tasks[i].get();
            } catch (...) {
            }
            m_retired_audio_tasks.erase(m_retired_audio_tasks.begin() + i);
        }
        for (std::size_t i = 0; i < m_retired_chart_tasks.size();) {
            if (!m_retired_chart_tasks[i].isReady()) {
                ++i;
                continue;
            }
            try {
                (void)m_retired_chart_tasks[i].get();
            } catch (...) {
            }
            m_retired_chart_tasks.erase(m_retired_chart_tasks.begin() + i);
        }
    }

    void GameLoader::fail(String message) {
        m_error = std::move(message);
        m_state = State::Failed;
        Logger << U"[Game] " + m_error;
    }
}
