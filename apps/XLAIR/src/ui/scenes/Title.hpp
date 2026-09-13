#pragma once

#include "ui/Scene.hpp"

namespace xlair::ui::scenes {
    class Title final : public SceneBase {
    public:
        explicit Title(const InitData& init);

        void draw() const override;

    private:
        Font m_font{ 28 };
    };
}
