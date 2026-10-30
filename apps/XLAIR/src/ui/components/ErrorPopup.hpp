#pragma once

#include "Common.hpp"

namespace xlair::ui::components {
    class ErrorPopup {
    public:
        void show(String title, String message, double duration_seconds);
        void update(double delta_seconds);
        void draw(const Vec2& center) const;

        [[nodiscard]]
        bool visible() const noexcept;

    private:
        String m_title;
        String m_message;
        double m_elapsed = 0.0;
        double m_duration = 0.0;
    };
}
