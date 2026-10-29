#pragma once

#include "CardCarousel.hpp"
#include "ui/input/NameEntry.hpp"

namespace xlair::ui::components {
    class NameEntryPanel {
    public:
        NameEntryPanel();

        void animateMove(int32 direction);
        void update(double delta_seconds);
        void draw(const input::NameEntry& entry) const;

    private:
        CardCarousel m_carousel;
    };
}
