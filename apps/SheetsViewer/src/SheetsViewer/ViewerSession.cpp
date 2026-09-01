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

        [[nodiscard]]
        s3d::String FormatDiagnostic(const sheets::Diagnostic& diagnostic) {
            if (diagnostic.path.isEmpty()) {
                return diagnostic.message;
            }

            s3d::String location = s3d::FileSystem::RelativePath(diagnostic.path);
            if (diagnostic.line) {
                location += U":" + s3d::Format(*diagnostic.line);
                if (diagnostic.column) {
                    location += U":" + s3d::Format(*diagnostic.column);
                }
            }
            return location + U": " + diagnostic.message;
        }
    }

    ViewerSession::~ViewerSession() {
        waitForTasks();
        releaseAssets();
    }

    bool ViewerSession::loadMetadata(const s3d::FilePath& path) {
        return beginMetadataLoading(path, s3d::none);
    }

    bool ViewerSession::reloadMetadata() {
        if (!m_metadata || isLoading()) {
            return false;
        }

        const s3d::FilePath path = m_metadata->source_path;
        s3d::Optional<s3d::String> difficulty_id;
        if (m_selected_difficulty_position && *m_selected_difficulty_position < m_metadata->difficulties.size()) {
            difficulty_id = m_metadata->difficulties[*m_selected_difficulty_position].id;
        }
        return beginMetadataLoading(path, difficulty_id);
    }

    bool
    ViewerSession::beginMetadataLoading(const s3d::FilePath& path, const s3d::Optional<s3d::String>& difficulty_id) {
        if (path.isEmpty() || isLoading()) {
            return false;
        }

        clear();
        m_pending_difficulty_id = difficulty_id;
        m_state = State::LoadingMetadata;
        m_metadata_task = s3d::AsyncTask<sheets::Result<sheets::Metadata>>{ [path]() {
            return sheets::LoadMetadata(path);
        } };
        return true;
    }

    bool ViewerSession::selectDifficulty(const std::size_t position) {
        if (!m_metadata || isLoading()) {
            return false;
        }
        return beginChartLoading(position);
    }

    void ViewerSession::update(const double delta_seconds) {
        switch (m_state) {
            case State::LoadingMetadata:
                updateMetadataLoading();
                break;
            case State::LoadingAssets:
                updateAssetLoading();
                break;
            case State::LoadingChart:
                updateChartLoading();
                break;
            default:
                break;
        }

        updatePlayback(delta_seconds);
    }

    void ViewerSession::updateMetadataLoading() {
        if (!m_metadata_task.isReady()) {
            return;
        }

        auto result = m_metadata_task.get();
        m_metadata_diagnostics = std::move(result.diagnostics);
        refreshDiagnostics();
        if (!result) {
            m_state = State::Failed;
            if (m_metadata_diagnostics.isEmpty()) {
                pushEvent(EventType::Error, U"Failed to load metadata");
            } else {
                publishDiagnostics(m_metadata_diagnostics);
            }
            return;
        }

        m_metadata = std::move(*result);
        publishDiagnostics(m_metadata_diagnostics);
        pushEvent(EventType::Success, U"Loaded metadata: " + s3d::FileSystem::RelativePath(m_metadata->source_path));
        beginAssetLoading();
    }

    void ViewerSession::beginAssetLoading() {
        m_state = State::LoadingAssets;
        m_music_loading = false;
        m_jacket_loading = false;

        if (!m_metadata->music.isEmpty()) {
            if (s3d::AudioAsset::Register(MusicAssetName, m_metadata->music)) {
                m_music_loading = true;
                s3d::AudioAsset::LoadAsync(MusicAssetName);
            } else {
                addAssetWarning(
                    U"Failed to register the music file. Manual transport will be used.",
                    m_metadata->music
                );
            }
        }

        if (!m_metadata->jacket.isEmpty()) {
            if (s3d::TextureAsset::Register(JacketAssetName, m_metadata->jacket)) {
                m_jacket_loading = true;
                s3d::TextureAsset::LoadAsync(JacketAssetName);
            } else {
                addAssetWarning(U"Failed to register the jacket image.", m_metadata->jacket);
            }
        }

        updateAssetLoading();
    }

    void ViewerSession::updateAssetLoading() {
        if (m_music_loading && s3d::AudioAsset::IsReady(MusicAssetName)) {
            s3d::Audio audio = s3d::AudioAsset{ MusicAssetName };
            if (audio) {
                m_audio = std::move(audio);
            } else {
                addAssetWarning(U"Failed to load the music file. Manual transport will be used.", m_metadata->music);
            }
            m_music_loading = false;
        }

        if (m_jacket_loading && s3d::TextureAsset::IsReady(JacketAssetName)) {
            s3d::Texture jacket = s3d::TextureAsset{ JacketAssetName };
            if (jacket) {
                m_jacket = std::move(jacket);
            } else {
                addAssetWarning(U"Failed to load the jacket image.", m_metadata->jacket);
            }
            m_jacket_loading = false;
        }

        if (m_music_loading || m_jacket_loading) {
            return;
        }

        pushEvent(EventType::Success, U"Loaded assets");
        if (m_metadata->difficulties.isEmpty()) {
            m_state = State::Ready;
            m_pending_difficulty_id.reset();
            return;
        }

        std::size_t position = 0;
        if (m_pending_difficulty_id) {
            const auto difficulty = std::find_if(
                m_metadata->difficulties.begin(),
                m_metadata->difficulties.end(),
                [this](const sheets::Difficulty& candidate) {
                    return candidate.id == *m_pending_difficulty_id;
                }
            );
            if (difficulty != m_metadata->difficulties.end()) {
                position = static_cast<std::size_t>(std::distance(m_metadata->difficulties.begin(), difficulty));
            }
        }
        m_pending_difficulty_id.reset();
        (void)beginChartLoading(position);
    }

    bool ViewerSession::beginChartLoading(const std::size_t position) {
        stopPlayback();
        m_chart.reset();
        m_projection.reset();
        m_chart_diagnostics.clear();
        m_selected_difficulty_position.reset();
        m_chart_end_sample = 0;

        if (!m_metadata || position >= m_metadata->difficulties.size()) {
            m_chart_diagnostics.push_back(
                {
                    .severity = sheets::DiagnosticSeverity::Error,
                    .message = U"The selected difficulty does not exist.",
                }
            );
            refreshDiagnostics();
            publishDiagnostics(m_chart_diagnostics);
            m_state = State::Failed;
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
            publishDiagnostics(m_chart_diagnostics);
            m_state = State::Failed;
            return false;
        }

        const s3d::FilePath source_path = difficulty.src;
        const s3d::int64 sample_rate = m_audio ? m_audio->sampleRate() : 44'100;
        const double offset_seconds = m_metadata->music_offset_seconds;
        m_selected_difficulty_position = position;
        m_state = State::LoadingChart;
        m_chart_task = s3d::AsyncTask<sheets::Result<sheets::Chart>>{ [source_path, sample_rate, offset_seconds]() {
            return sheets::LoadChart(
                source_path,
                {
                    .sample_rate = sample_rate,
                    .offset_seconds = offset_seconds,
                }
            );
        } };
        return true;
    }

    void ViewerSession::updateChartLoading() {
        if (!m_chart_task.isReady()) {
            return;
        }

        auto result = m_chart_task.get();
        m_chart_diagnostics = std::move(result.diagnostics);
        refreshDiagnostics();
        publishDiagnostics(m_chart_diagnostics);
        if (!result) {
            m_state = State::Failed;
            if (m_chart_diagnostics.isEmpty()) {
                pushEvent(EventType::Error, U"Failed to load the selected difficulty");
            }
            return;
        }

        m_chart = std::move(*result);
        m_projection.emplace(*m_chart);
        m_chart_end_sample = ChartEndSample(*m_chart);
        m_state = State::Ready;

        const auto& difficulty = m_metadata->difficulties[*m_selected_difficulty_position];
        pushEvent(EventType::Success, U"Loaded difficulty: {}"_fmt(difficulty.id));
        pushEvent(EventType::Information, U"Total combo: {}"_fmt(m_chart->total_combo));
    }

    void ViewerSession::updatePlayback(const double delta_seconds) {
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
        if (!m_chart || isLoading()) {
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
        if (isLoading()) {
            return;
        }

        const s3d::int64 clamped = std::clamp<s3d::int64>(sample, 0, durationSamples());
        if (m_audio) {
            m_audio->seekSamples(static_cast<std::size_t>(clamped));
        } else {
            m_manual_sample = clamped;
        }
    }

    void ViewerSession::clear() {
        waitForTasks();
        releaseAssets();

        m_state = State::Empty;
        m_pending_difficulty_id.reset();
        m_music_loading = false;
        m_jacket_loading = false;
        m_metadata.reset();
        m_chart.reset();
        m_projection.reset();
        m_selected_difficulty_position.reset();
        m_metadata_diagnostics.clear();
        m_asset_diagnostics.clear();
        m_chart_diagnostics.clear();
        m_diagnostics.clear();
        m_events.clear();
        m_manual_sample = 0.0L;
        m_chart_end_sample = 0;
        m_manual_playing = false;
    }

    ViewerSession::State ViewerSession::state() const noexcept {
        return m_state;
    }

    bool ViewerSession::isLoading() const noexcept {
        return m_state == State::LoadingMetadata || m_state == State::LoadingAssets || m_state == State::LoadingChart;
    }

    s3d::StringView ViewerSession::loadingMessage() const noexcept {
        switch (m_state) {
            case State::LoadingMetadata:
                return U"Loading metadata...";
            case State::LoadingAssets:
                return U"Loading music and jacket...";
            case State::LoadingChart:
                return U"Loading chart...";
            default:
                return U"";
        }
    }

    s3d::Array<ViewerSession::Event> ViewerSession::takeEvents() {
        s3d::Array<Event> events = std::move(m_events);
        m_events.clear();
        return events;
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

    void ViewerSession::waitForTasks() {
        if (m_metadata_task.isValid()) {
            m_metadata_task.wait();
        }
        if (m_chart_task.isValid()) {
            m_chart_task.wait();
        }
    }

    void ViewerSession::releaseAssets() {
        if (m_music_loading && s3d::AudioAsset::IsRegistered(MusicAssetName)) {
            s3d::AudioAsset::Wait(MusicAssetName);
        }
        if (m_jacket_loading && s3d::TextureAsset::IsRegistered(JacketAssetName)) {
            s3d::TextureAsset::Wait(JacketAssetName);
        }

        if (m_audio) {
            m_audio->stop();
        }
        m_audio.reset();
        m_jacket.reset();

        if (s3d::AudioAsset::IsRegistered(MusicAssetName)) {
            s3d::AudioAsset::Unregister(MusicAssetName);
        }
        if (s3d::TextureAsset::IsRegistered(JacketAssetName)) {
            s3d::TextureAsset::Unregister(JacketAssetName);
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
        refreshDiagnostics();
        publishDiagnostics({ m_asset_diagnostics.back() });
    }

    void ViewerSession::publishDiagnostics(const s3d::Array<sheets::Diagnostic>& diagnostics) {
        if (diagnostics.isEmpty()) {
            return;
        }

        const auto error =
            std::find_if(diagnostics.begin(), diagnostics.end(), [](const sheets::Diagnostic& diagnostic) {
                return diagnostic.severity == sheets::DiagnosticSeverity::Error;
            });
        const auto& representative = error != diagnostics.end() ? *error : diagnostics.front();
        s3d::String message = FormatDiagnostic(representative);
        if (1 < diagnostics.size()) {
            message += U" (and {} more)"_fmt(diagnostics.size() - 1);
        }
        const EventType type =
            representative.severity == sheets::DiagnosticSeverity::Error ? EventType::Error : EventType::Warning;
        pushEvent(type, std::move(message));
    }

    void ViewerSession::pushEvent(const EventType type, s3d::String message) {
        m_events.push_back({ .type = type, .message = std::move(message) });
    }

    void ViewerSession::refreshDiagnostics() {
        m_diagnostics = m_metadata_diagnostics;
        m_diagnostics.append(m_asset_diagnostics);
        m_diagnostics.append(m_chart_diagnostics);
    }
}
