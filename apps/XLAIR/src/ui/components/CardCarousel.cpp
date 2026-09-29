#include "CardCarousel.hpp"

#include "ui/primitives/Arrow.hpp"

namespace xlair::ui::components {
    namespace {
        constexpr double SideCardScale = 0.88;
        constexpr double SelectedCardMargin = 50.0;
        constexpr double CardSpacing = 50.0;
        constexpr double SmoothTime = 0.1;
    }

    CardCarousel::CardCarousel(const SizeF& card_size, const Vec2& center, const double viewport_width)
        : m_card_size{ card_size }, m_center{ center }, m_viewport_width{ viewport_width } {}

    void CardCarousel::animateMove(const int32 direction) {
        if (direction != 0) {
            m_scroll_offset = direction < 0 ? 1.0 : -1.0;
        }
    }

    void CardCarousel::update(const double delta_seconds) {
        m_scroll_offset =
            Math::SmoothDamp(m_scroll_offset, 0.0, m_scroll_velocity, SmoothTime, unspecified, delta_seconds);
    }

    Array<CardPlacement> CardCarousel::layout(const std::size_t selected_index, const std::size_t item_count) const {
        Array<CardPlacement> placements;
        if (selected_index >= item_count) {
            return placements;
        }

        const SizeF side_card_size = m_card_size * SideCardScale;
        const double scroll = m_scroll_offset;
        const double scroll_abs = Abs(scroll);
        const double neighbor_gap = m_card_size.x / 2.0 + side_card_size.x / 2.0 + CardSpacing + SelectedCardMargin;

        const SizeF selected_size = m_card_size.lerp(side_card_size, scroll_abs);
        const double selected_x = m_center.x - neighbor_gap * scroll;
        placements.push_back(
            {
                .index = selected_index,
                .region = RectF{ Arg::center = Vec2{ selected_x, m_center.y }, selected_size },
                .selected = true,
            }
        );

        const auto place_side = [&](const int32 direction) {
            const double directional_scroll = direction * scroll;
            const double margin_factor = Min(1.0, 1.0 + directional_scroll);
            const double neighbor_scale = Clamp(directional_scroll, 0.0, 1.0);
            double x =
                selected_x + direction * (selected_size.x / 2.0 + CardSpacing + SelectedCardMargin * margin_factor);

            for (int64 index = static_cast<int64>(selected_index) + direction;
                 index >= 0 && index < static_cast<int64>(item_count);
                 index += direction) {
                if (x - direction * side_card_size.x > m_viewport_width || x - direction * side_card_size.x < 0) {
                    break;
                }

                SizeF card_size = side_card_size;
                if (index == static_cast<int64>(selected_index) + direction) {
                    card_size = side_card_size.lerp(m_card_size, neighbor_scale);
                    x += direction * (CardSpacing + SelectedCardMargin) * neighbor_scale;
                }

                placements.push_back(
                    {
                        .index = static_cast<std::size_t>(index),
                        .region =
                            RectF{ Arg::center = Vec2{ x + direction * card_size.x / 2.0, m_center.y }, card_size },
                        .selected = false,
                    }
                );
                x += direction * (CardSpacing + side_card_size.x);
            }
        };

        place_side(1);
        place_side(-1);
        return placements;
    }

    void CardCarousel::drawArrows(
        const std::size_t selected_index,
        const std::size_t item_count,
        const ColorF& color
    ) const {
        if (selected_index >= item_count) {
            return;
        }

        const Vec2 right = m_center.movedBy(m_card_size.x / 2.0 - 10, 0);
        const Vec2 left = m_center.movedBy(-m_card_size.x / 2.0 + 10, 0);
        if (selected_index + 1 < item_count) {
            primitives::DrawArrow(right, primitives::ArrowDirection::Right, color);
            primitives::DrawArrow(right.movedBy(30, 0), primitives::ArrowDirection::Right, color);
        }
        if (selected_index > 0) {
            primitives::DrawArrow(left, primitives::ArrowDirection::Left, color);
            primitives::DrawArrow(left.movedBy(-30, 0), primitives::ArrowDirection::Left, color);
        }
    }
}
