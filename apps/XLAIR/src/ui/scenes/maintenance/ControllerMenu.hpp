#pragma once

#include "Common.hpp"

namespace xlair::ui::scenes::maintenance {
    class ControllerMenu final : public Base {
    public:
        explicit ControllerMenu(const InitData& init);

        void update() override;
        void draw() const override;

    private:
        bool lightsSupported() const;
    };
}
