#pragma once

#include "maintenance/Base.hpp"

namespace xlair::ui::scenes {
    class Maintenance final : public maintenance::Base {
    public:
        explicit Maintenance(const InitData& init);

        void update() override;
        void draw() const override;
    };
}
