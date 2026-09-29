#pragma once

#include "Common.hpp"

namespace xlair::ui::components {
    struct CardPlacement {
        std::size_t index = 0;
        RectF region;
        bool selected = false;
    };

    // Selection and card content stay with the scene; this component handles presentation only.
    class CardCarousel {
    public:
        CardCarousel(const SizeF& card_size, const Vec2& center, double viewport_width);

        // Call after changing selection. A positive direction means moving to the right.
        void animateMove(int32 direction);
        void update(double delta_seconds);

        // Placements are returned in drawing order: selected card, right neighbors, left neighbors.
        [[nodiscard]]
        Array<CardPlacement> layout(std::size_t selected_index, std::size_t item_count) const;

        void drawArrows(std::size_t selected_index, std::size_t item_count, const ColorF& color) const;

    private:
        SizeF m_card_size;
        Vec2 m_center;
        double m_viewport_width = 0.0;
        double m_scroll_offset = 0.0;
        double m_scroll_velocity = 0.0;
    };
}
