#pragma once

#include "ui/Scene.hpp"

namespace xlair::ui::scenes {
    class Boot final : public SceneBase {
    public:
        explicit Boot(const InitData& init);

        void update() override;
        void draw() const override;

    private:
        void handleStateChange(app::flows::Boot::State previous);
        void updateJacketLoading();

        bool m_jacket_loading_started = false;
    };
}
