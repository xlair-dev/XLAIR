#pragma once

#include "app/flows/MusicSelect.hpp"
#include "ui/audio/MusicPreviewPlayer.hpp"

namespace xlair::ui {
    class MusicSelectContext {
    public:
        MusicSelectContext(const Array<sheets::Metadata>& catalog, double remaining_seconds);

        // Main updates this once per frame, including while Settings is active.
        void update(double delta_seconds);

        [[nodiscard]]
        app::flows::MusicSelect& flow() noexcept;

        [[nodiscard]]
        const app::flows::MusicSelect& flow() const noexcept;

        [[nodiscard]]
        double remainingSeconds() const noexcept;

    private:
        app::flows::MusicSelect m_flow;
        audio::MusicPreviewPlayer m_preview_player;
        double m_remaining_seconds = 0.0;
    };
}
