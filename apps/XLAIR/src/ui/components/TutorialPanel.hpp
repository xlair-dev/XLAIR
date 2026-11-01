#pragma once

#include "Common.hpp"

namespace xlair::ui::components {
    inline constexpr RectF TutorialGuideBounds{ 460, 120, 1000, 210 };

    enum class TutorialPanelKind {
        Prompt,
        Explain,
        Complete,
    };

    struct TutorialPanelData {
        TutorialPanelKind kind = TutorialPanelKind::Prompt;
        StringView title;
        StringView message;
    };

    void DrawTutorialPanel(const TutorialPanelData& data, double opacity = 1.0);
}
