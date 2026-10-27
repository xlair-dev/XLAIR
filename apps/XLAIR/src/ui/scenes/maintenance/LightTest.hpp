#pragma once

#include "Base.hpp"

#include "app/flows/LightTest.hpp"

namespace xlair::ui::scenes::maintenance {
    class LightTest final : public Base {
    public:
        explicit LightTest(const InitData& init);

        void update() override;
        void draw() const override;

    private:
        std::size_t m_selected_index = 0;
        std::unique_ptr<app::flows::LightTest> m_light_test;
    };
}
