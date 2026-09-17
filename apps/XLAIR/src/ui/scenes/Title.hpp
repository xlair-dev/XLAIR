#pragma once

#include "ui/Scene.hpp"

namespace xlair::ui::scenes {
    class Title final : public SceneBase {
    public:
        explicit Title(const InitData& init);
        ~Title() override;

        void update() override;
        void draw() const override;

    private:
        void startScan();

        Font m_font{ 28 };
        app::card::ScanRequest m_scan;
    };
}
