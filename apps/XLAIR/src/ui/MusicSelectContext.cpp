#include "MusicSelectContext.hpp"

namespace xlair::ui {
    MusicSelectContext::MusicSelectContext(const Array<sheets::Metadata>& catalog, const double remaining_seconds)
        : m_flow{ catalog }, m_remaining_seconds{ Max(0.0, remaining_seconds) } {}

    void MusicSelectContext::update(const double delta_seconds) {
        if (m_paused) {
            return;
        }

        m_remaining_seconds = Max(0.0, m_remaining_seconds - delta_seconds);

        if (const auto* music = m_flow.selectedMusic()) {
            m_preview_player.update(music->music, music->demo_start_seconds);
        } else {
            m_preview_player.stop();
        }
    }

    void MusicSelectContext::pause() {
        m_paused = true;
        m_preview_player.stop(SecondsF{ 0.25 });
    }

    void MusicSelectContext::resume() noexcept {
        m_paused = false;
    }

    app::flows::MusicSelect& MusicSelectContext::flow() noexcept {
        return m_flow;
    }

    const app::flows::MusicSelect& MusicSelectContext::flow() const noexcept {
        return m_flow;
    }

    double MusicSelectContext::remainingSeconds() const noexcept {
        return m_remaining_seconds;
    }

    std::size_t MusicSelectContext::selectedSettingIndex() const noexcept {
        return m_selected_setting_index;
    }

    void MusicSelectContext::setSelectedSettingIndex(const std::size_t index) noexcept {
        m_selected_setting_index = index;
    }
}
