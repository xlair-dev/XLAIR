#include "LatencyCalibration.hpp"

#include "ui/audio/AudioBus.hpp"
#include "ui/audio/CalibrationTrack.hpp"
#include "ui/components/LatencyCalibrationView.hpp"

namespace xlair::ui::scenes::maintenance {
    namespace {
        constexpr auto ApplyLatencyButton = app::controller::MaintenanceButton::Button2;
        constexpr auto ResetLatencyButton = app::controller::MaintenanceButton::Button4;
    }

    LatencyCalibration::LatencyCalibration(const InitData& init) : Base{ init } {
        reset();
    }

    LatencyCalibration::~LatencyCalibration() {
        m_latency_audio.stop();
    }

    void LatencyCalibration::reset() {
        m_latency_audio.stop();
        m_latency_audio = audio::CreateCalibrationTrack();
        m_latency_calibration.reset();
        m_latency_time_ms = 0.0;
        m_latency_applied = false;
        // Use the same bus/clock as gameplay music, not the UI sound bus.
        m_latency_audio.play(audio::Music);
    }

    void LatencyCalibration::update() {
        if (!navigate(SceneState::MaintenanceController)) {
            return;
        }
        const auto* controller = getData().application->controller();
        if (controller && controller->maintenanceButton(ResetLatencyButton).down()) {
            reset();
            return;
        }
        if (!m_latency_audio || !m_latency_audio.isPlaying()) {
            return;
        }
        // samplesPlayed includes loops. posSample would jump backwards at each loop.
        m_latency_time_ms = m_latency_audio.samplesPlayed() * 1000.0 / m_latency_audio.sampleRate();
        if (controller && controller->maintenanceButton(ApplyLatencyButton).down()) {
            if (m_latency_calibration.ready()) {
                getData().application->setInputLatencyOffsetSeconds(*m_latency_calibration.averageMs() / 1000.0);
                m_latency_applied = true;
            }
            return;
        }

        bool tapped = KeySpace.down();
        if (controller) {
            for (std::size_t index = 0; index < app::controller::TouchZoneCount; ++index) {
                tapped = tapped || controller->touchZone(index).down();
            }
            for (std::size_t index = 0; index < app::controller::SideButtonCount; ++index) {
                tapped = tapped || controller->sideButton(static_cast<app::controller::SideButton>(index)).down();
            }
        }
        if (tapped && m_latency_calibration.tap(m_latency_time_ms)) {
            m_latency_applied = false;
        }
    }

    void LatencyCalibration::draw() const {
        drawFrame(U"入力遅延補正", U"Space / コントローラ: タップ    F3: 適用    F5: 再計測    ←: 戻る    F2: Title");
        const auto& config = getData().application->config();
        components::DrawLatencyCalibration(
            { .calibration = m_latency_calibration,
              .time_ms = m_latency_time_ms,
              .applied_offset_ms = config ? Optional<double>{ config->input.latency_offset_seconds * 1000.0 } : none,
              .audio_playing = m_latency_audio && m_latency_audio.isPlaying(),
              .applied = m_latency_applied },
            Scene::Size()
        );
    }
}
