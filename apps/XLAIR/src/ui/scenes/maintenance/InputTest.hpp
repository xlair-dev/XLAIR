#pragma once

#include "Common.hpp"

namespace xlair::ui::scenes::maintenance {
    class InputTest final : public Base {
    public:
        explicit InputTest(const InitData& init);

        void update() override;
        void draw() const override;

    private:
        bool m_highlight_touch_threshold = false;
    };
}
