#include "ViewerSession.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace xlair::sheets_viewer {
    namespace {
        [[nodiscard]]
        s3d::int64 ChartEndSample(const sheets::Chart& chart) {
            s3d::int64 end_sample = 0;
            const auto include = [&end_sample](const s3d::int64 sample) {
                end_sample = std::max(end_sample, sample);
            };

            for (const auto& note : chart.slider_notes) {
                include(note.sample);
            }
            for (const auto& hold : chart.slider_holds) {
                for (const auto& point : hold.points) {
                    include(point.sample);
                }
                for (const auto& point : hold.judge_points) {
                    include(point.sample);
                }
            }
            for (const auto& note : chart.side_notes) {
                include(note.sample);
            }
            for (const auto& hold : chart.side_holds) {
                for (const auto& point : hold.points) {
                    include(point.sample);
                }
                for (const auto sample : hold.judge_samples) {
                    include(sample);
                }
            }

            const s3d::int64 available = std::numeric_limits<s3d::int64>::max() - end_sample;
            const s3d::int64 tail = std::min(chart.sample_rate, available / 2) * 2;
            return end_sample + tail;
        }
    }

    bool ViewerSession::loadMetadata(const s3d::FilePath& path) {
        clear();

        auto result = sheets::LoadMetadata(path);
        if (!result) {
            m_diagnostics = std::move(result.diagnostics);
            return false;
        }

        m_metadata = std::move(*result);
        loadAssets();

        if (m_metadata->difficulties.isEmpty()) {
            refreshDiagnostics();
            return true;
        }

        return selectDifficulty(0);
    }

    bool ViewerSession::selectDifficulty(const std::size_t position) {
        m_chart.reset();
        m_projection.reset();
        m_chart_diagnostics.clear();
        m_selected_difficulty_position.reset();
        m_manual_sample = 0.0L;
        m_manual_playing = false;
        m_chart_end_sample = 0;

        if (!m_metadata || position >= m_metadata->difficulties.size()) {
            m_chart_diagnostics.push_back(
                {
                    .severity = sheets::DiagnosticSeverity::Error,
                    .message = U"The selected difficulty does not exist.",
                }
            );
            refreshDiagnostics();
            return false;
        }

        const auto& difficulty = m_metadata->difficulties[position];
        if (difficulty.src.isEmpty()) {
            m_chart_diagnostics.push_back(
                {
                    .severity = sheets::DiagnosticSeverity::Error,
                    .message = U"The selected difficulty does not specify a chart file.",
                    .path = m_metadata->source_path,
                }
            );
            refreshDiagnostics();
            return false;
        }

        const s3d::int64 sample_rate = m_audio ? m_audio->sampleRate() : 44'100;
        auto result = sheets::LoadChart(
            difficulty.src,
            {
                .sample_rate = sample_rate,
                .offset_seconds = m_metadata->music_offset_seconds,
            }
        );
        m_chart_diagnostics = std::move(result.diagnostics);
        if (!result) {
            refreshDiagnostics();
            return false;
        }

        m_chart = std::move(*result);
        m_projection.emplace(*m_chart);
        m_chart_end_sample = ChartEndSample(*m_chart);
        m_selected_difficulty_position = position;
        refreshDiagnostics();
        return true;
    }

    void ViewerSession::update(const double delta_seconds) {
        if (m_audio || !m_manual_playing || !m_chart || !std::isfinite(delta_seconds) || delta_seconds <= 0.0) {
            return;
        }

        m_manual_sample += static_cast<long double>(delta_seconds) * m_chart->sample_rate;
        if (m_manual_sample >= durationSamples()) {
            m_manual_sample = durationSamples();
            m_manual_playing = false;
        }
    }

    void ViewerSession::togglePlayback() {
        if (!m_chart) {
            return;
        }

        if (m_audio) {
            if (m_audio->isPlaying()) {
                m_audio->pause();
            } else {
                m_audio->play();
            }
            return;
        }

        if (currentSample() >= durationSamples()) {
            m_manual_sample = 0.0L;
        }
        m_manual_playing = !m_manual_playing;
    }

    void ViewerSession::stopPlayback() {
        if (m_audio) {
            m_audio->stop();
        }
        m_manual_sample = 0.0L;
        m_manual_playing = false;
    }

    void ViewerSession::seekSample(const s3d::int64 sample) {
        const s3d::int64 clamped = std::clamp<s3d::int64>(sample, 0, durationSamples());
        if (m_audio) {
            m_audio->seekSamples(static_cast<std::size_t>(clamped));
        } else {
            m_manual_sample = clamped;
        }
    }

    void ViewerSession::clear() {
        if (m_audio) {
            m_audio->stop();
        }

        m_metadata.reset();
        m_chart.reset();
        m_projection.reset();
        m_audio.reset();
        m_jacket.reset();
        m_selected_difficulty_position.reset();
        m_asset_diagnostics.clear();
        m_chart_diagnostics.clear();
        m_diagnostics.clear();
        m_manual_sample = 0.0L;
        m_chart_end_sample = 0;
        m_manual_playing = false;
    }

    const s3d::Optional<sheets::Metadata>& ViewerSession::metadata() const noexcept {
        return m_metadata;
    }

    const s3d::Optional<sheets::Chart>& ViewerSession::chart() const noexcept {
        return m_chart;
    }

    const s3d::Optional<playfield::ChartProjection>& ViewerSession::projection() const noexcept {
        return m_projection;
    }

    const s3d::Optional<s3d::Audio>& ViewerSession::audio() const noexcept {
        return m_audio;
    }

    const s3d::Optional<s3d::Texture>& ViewerSession::jacket() const noexcept {
        return m_jacket;
    }

    const s3d::Optional<std::size_t>& ViewerSession::selectedDifficultyPosition() const noexcept {
        return m_selected_difficulty_position;
    }

    const s3d::Array<sheets::Diagnostic>& ViewerSession::diagnostics() const noexcept {
        return m_diagnostics;
    }

    s3d::int64 ViewerSession::currentSample() const {
        if (m_audio) {
            return m_audio->posSample();
        }

        constexpr long double minimum = std::numeric_limits<s3d::int64>::min();
        constexpr long double maximum = std::numeric_limits<s3d::int64>::max();
        return static_cast<s3d::int64>(std::clamp(std::round(m_manual_sample), minimum, maximum));
    }

    s3d::int64 ViewerSession::durationSamples() const {
        if (m_audio) {
            return static_cast<s3d::int64>(
                std::min<std::size_t>(m_audio->samples(), std::numeric_limits<s3d::int64>::max())
            );
        }
        return m_chart_end_sample;
    }

    bool ViewerSession::isPlaying() const {
        return m_audio ? m_audio->isPlaying() : m_manual_playing;
    }

    void ViewerSession::loadAssets() {
        if (!m_metadata->music.isEmpty()) {
            s3d::Audio audio{ m_metadata->music };
            if (audio) {
                m_audio = std::move(audio);
            } else {
                addAssetWarning(U"Failed to load the music file. Manual transport will be used.", m_metadata->music);
            }
        }

        if (!m_metadata->jacket.isEmpty()) {
            s3d::Texture jacket{ m_metadata->jacket };
            if (jacket) {
                m_jacket = std::move(jacket);
            } else {
                addAssetWarning(U"Failed to load the jacket image.", m_metadata->jacket);
            }
        }
    }

    void ViewerSession::addAssetWarning(s3d::String message, s3d::FilePath path) {
        m_asset_diagnostics.push_back(
            {
                .severity = sheets::DiagnosticSeverity::Warning,
                .message = std::move(message),
                .path = std::move(path),
            }
        );
    }

    void ViewerSession::refreshDiagnostics() {
        m_diagnostics = m_asset_diagnostics;
        m_diagnostics.append(m_chart_diagnostics);
    }
}
