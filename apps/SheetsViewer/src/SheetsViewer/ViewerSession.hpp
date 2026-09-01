#pragma once

#include <Playfield.hpp>
#include <SheetsAnalyzer.hpp>

namespace xlair::sheets_viewer {
    class ViewerSession {
    public:
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
        [[nodiscard]]
        bool loadMetadataAndSelect(const s3d::FilePath& path, const s3d::Optional<s3d::String>& difficulty_id);

        void loadAssets();
        void addAssetWarning(s3d::String message, s3d::FilePath path);
        void refreshDiagnostics();

        s3d::Optional<sheets::Metadata> m_metadata;
        s3d::Optional<sheets::Chart> m_chart;
        s3d::Optional<playfield::ChartProjection> m_projection;
        s3d::Optional<s3d::Audio> m_audio;
        s3d::Optional<s3d::Texture> m_jacket;
        s3d::Optional<std::size_t> m_selected_difficulty_position;
        s3d::Array<sheets::Diagnostic> m_asset_diagnostics;
        s3d::Array<sheets::Diagnostic> m_chart_diagnostics;
        s3d::Array<sheets::Diagnostic> m_diagnostics;
        long double m_manual_sample = 0.0L;
        s3d::int64 m_chart_end_sample = 0;
        bool m_manual_playing = false;
    };
}
