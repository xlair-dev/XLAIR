#pragma once

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
            s3d::String message;
        };

        ViewerSession() = default;
        ~ViewerSession();

        ViewerSession(const ViewerSession&) = delete;
        ViewerSession& operator=(const ViewerSession&) = delete;

        [[nodiscard]]
        bool loadMetadata(const s3d::FilePath& path);

        [[nodiscard]]
        bool reloadMetadata();

        [[nodiscard]]
        bool selectDifficulty(std::size_t position);

        void update(double delta_seconds);
        void togglePlayback();
        void stopPlayback();
        void seekSample(s3d::int64 sample);
        void clear();

        [[nodiscard]]
        State state() const noexcept;

        [[nodiscard]]
        bool isLoading() const noexcept;

        [[nodiscard]]
        s3d::StringView loadingMessage() const noexcept;

        [[nodiscard]]
        s3d::Array<Event> takeEvents();

        [[nodiscard]]
        const s3d::Optional<sheets::Metadata>& metadata() const noexcept;

        [[nodiscard]]
        const s3d::Optional<sheets::Chart>& chart() const noexcept;

        [[nodiscard]]
        const s3d::Optional<playfield::ChartProjection>& projection() const noexcept;

        [[nodiscard]]
        const s3d::Optional<s3d::Audio>& audio() const noexcept;

        [[nodiscard]]
        const s3d::Optional<s3d::Texture>& jacket() const noexcept;

        [[nodiscard]]
        const s3d::Optional<std::size_t>& selectedDifficultyPosition() const noexcept;

        [[nodiscard]]
        const s3d::Array<sheets::Diagnostic>& diagnostics() const noexcept;

        [[nodiscard]]
        s3d::int64 currentSample() const;

        [[nodiscard]]
        s3d::int64 durationSamples() const;

        [[nodiscard]]
        bool isPlaying() const;

    private:
        inline static constexpr s3d::StringView MusicAssetName = U"SheetsViewer.Music";
        inline static constexpr s3d::StringView JacketAssetName = U"SheetsViewer.Jacket";

        [[nodiscard]]
        bool beginMetadataLoading(const s3d::FilePath& path, const s3d::Optional<s3d::String>& difficulty_id);

        void updateMetadataLoading();
        void beginAssetLoading();
        void updateAssetLoading();

        [[nodiscard]]
        bool beginChartLoading(std::size_t position);

        void updateChartLoading();
        void updatePlayback(double delta_seconds);
        void waitForTasks();
        void releaseAssets();
        void addAssetWarning(s3d::String message, s3d::FilePath path);
        void publishDiagnostics(const s3d::Array<sheets::Diagnostic>& diagnostics);
        void pushEvent(EventType type, s3d::String message);
        void refreshDiagnostics();

        State m_state = State::Empty;
        s3d::AsyncTask<sheets::Result<sheets::Metadata>> m_metadata_task;
        s3d::AsyncTask<sheets::Result<sheets::Chart>> m_chart_task;
        s3d::Optional<s3d::String> m_pending_difficulty_id;
        bool m_music_loading = false;
        bool m_jacket_loading = false;
        s3d::Optional<sheets::Metadata> m_metadata;
        s3d::Optional<sheets::Chart> m_chart;
        s3d::Optional<playfield::ChartProjection> m_projection;
        s3d::Optional<s3d::Audio> m_audio;
        s3d::Optional<s3d::Texture> m_jacket;
        s3d::Optional<std::size_t> m_selected_difficulty_position;
        s3d::Array<sheets::Diagnostic> m_metadata_diagnostics;
        s3d::Array<sheets::Diagnostic> m_asset_diagnostics;
        s3d::Array<sheets::Diagnostic> m_chart_diagnostics;
        s3d::Array<sheets::Diagnostic> m_diagnostics;
        s3d::Array<Event> m_events;
        long double m_manual_sample = 0.0L;
        s3d::int64 m_chart_end_sample = 0;
        bool m_manual_playing = false;
    };
}
