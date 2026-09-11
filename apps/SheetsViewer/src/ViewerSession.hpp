#pragma once

#include "Common.hpp"

#include <Playfield.hpp>
#include <SheetsAnalyzer.hpp>

namespace xlair::sheets_viewer {
    class ViewerSession {
    public:
        enum class State {
            Empty,
            LoadingMetadata,
            LoadingAssets,
            LoadingChart,
            Ready,
            Failed,
        };

        enum class EventType {
            Information,
            Success,
            Warning,
            Error,
        };

        struct Event {
            EventType type = EventType::Information;
            String message;
        };

        ViewerSession() = default;
        ~ViewerSession();

        ViewerSession(const ViewerSession&) = delete;
        ViewerSession& operator=(const ViewerSession&) = delete;

        [[nodiscard]]
        bool loadMetadata(const FilePath& path);

        [[nodiscard]]
        bool reloadMetadata();

        [[nodiscard]]
        bool selectDifficulty(std::size_t position);

        void update(double delta_seconds);
        void togglePlayback();
        void stopPlayback();
        void seekSample(int64 sample);
        void clear();

        [[nodiscard]]
        State state() const noexcept;

        [[nodiscard]]
        bool isLoading() const noexcept;

        [[nodiscard]]
        StringView loadingMessage() const noexcept;

        [[nodiscard]]
        Array<Event> takeEvents();

        [[nodiscard]]
        const Optional<sheets::Metadata>& metadata() const noexcept;

        [[nodiscard]]
        const Optional<sheets::Chart>& chart() const noexcept;

        [[nodiscard]]
        const Optional<playfield::ChartProjection>& projection() const noexcept;

        [[nodiscard]]
        const Optional<Audio>& audio() const noexcept;

        [[nodiscard]]
        const Optional<Texture>& jacket() const noexcept;

        [[nodiscard]]
        const Optional<std::size_t>& selectedDifficultyPosition() const noexcept;

        [[nodiscard]]
        const Array<sheets::Diagnostic>& diagnostics() const noexcept;

        [[nodiscard]]
        int64 currentSample() const;

        [[nodiscard]]
        int64 durationSamples() const;

        [[nodiscard]]
        bool isPlaying() const;

    private:
        inline static constexpr StringView MusicAssetName = U"SheetsViewer.Music";
        inline static constexpr StringView JacketAssetName = U"SheetsViewer.Jacket";

        [[nodiscard]]
        bool beginMetadataLoading(const FilePath& path, const Optional<String>& difficulty_id);

        void updateMetadataLoading();
        void beginAssetLoading();
        void updateAssetLoading();

        [[nodiscard]]
        bool beginChartLoading(std::size_t position);

        void updateChartLoading();
        void updatePlayback(double delta_seconds);
        void waitForTasks();
        void releaseAssets();
        void addAssetWarning(String message, FilePath path);
        void publishDiagnostics(const Array<sheets::Diagnostic>& diagnostics);
        void pushEvent(EventType type, String message);
        void refreshDiagnostics();

        State m_state = State::Empty;
        AsyncTask<sheets::Result<sheets::Metadata>> m_metadata_task;
        AsyncTask<sheets::Result<sheets::Chart>> m_chart_task;
        Optional<String> m_pending_difficulty_id;
        bool m_music_loading = false;
        bool m_jacket_loading = false;
        Optional<sheets::Metadata> m_metadata;
        Optional<sheets::Chart> m_chart;
        Optional<playfield::ChartProjection> m_projection;
        Optional<Audio> m_audio;
        Optional<Texture> m_jacket;
        Optional<std::size_t> m_selected_difficulty_position;
        Array<sheets::Diagnostic> m_metadata_diagnostics;
        Array<sheets::Diagnostic> m_asset_diagnostics;
        Array<sheets::Diagnostic> m_chart_diagnostics;
        Array<sheets::Diagnostic> m_diagnostics;
        Array<Event> m_events;
        long double m_manual_sample = 0.0L;
        int64 m_chart_end_sample = 0;
        bool m_manual_playing = false;
    };
}
