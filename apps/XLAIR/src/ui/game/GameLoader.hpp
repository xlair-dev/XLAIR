#pragma once

#include "Common.hpp"
#include "ui/game/GameSelection.hpp"

#include <SheetsAnalyzer.hpp>

namespace xlair::ui::game {
    class GameLoader {
    public:
        enum class State {
            Idle,
            LoadingAudio,
            LoadingChart,
            Ready,
            Failed,
        };

        ~GameLoader();

        GameLoader(const GameLoader&) = delete;
        GameLoader& operator=(const GameLoader&) = delete;

        GameLoader() = default;

        void start(const GameSelection& selection);
        void update();
        void cancel();

        [[nodiscard]]
        State state() const noexcept;

        [[nodiscard]]
        const String& error() const noexcept;

        [[nodiscard]]
        const Array<sheets::Diagnostic>& diagnostics() const noexcept;

        [[nodiscard]]
        const Audio& audio() const noexcept;

        [[nodiscard]]
        const Optional<sheets::Chart>& chart() const noexcept;

    private:
        void updateAudio();
        void updateChart();
        void reapRetiredTasks();
        void fail(String message);

        State m_state = State::Idle;
        String m_error;
        Array<sheets::Diagnostic> m_diagnostics;
        Audio m_audio;
        Optional<sheets::Chart> m_chart;
        AsyncTask<Audio> m_audio_task;
        AsyncTask<sheets::Result<sheets::Chart>> m_chart_task;
        Array<AsyncTask<Audio>> m_retired_audio_tasks;
        Array<AsyncTask<sheets::Result<sheets::Chart>>> m_retired_chart_tasks;
        FilePath m_chart_path;
        double m_offset_seconds = 0.0;
    };
}
