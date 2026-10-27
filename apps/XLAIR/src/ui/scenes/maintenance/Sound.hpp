#pragma once

#include "Common.hpp"

namespace xlair::ui::scenes::maintenance {
    class Sound final : public Base {
    public:
        explicit Sound(const InitData& init);

        void update() override;
        void draw() const override;

    private:
        std::size_t m_selected_index = 0;
        ValueRepeat m_value_repeat;
    };
}
