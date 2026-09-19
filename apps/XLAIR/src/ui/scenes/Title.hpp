#pragma once

#include "app/flows/Login.hpp"
#include "ui/Scene.hpp"

namespace xlair::ui::scenes {
    class Title final : public SceneBase {
    public:
        explicit Title(const InitData& init);

        void update() override;
        void draw() const override;

    private:
        void reportState() const;

        Font m_font{ 28 };
        std::unique_ptr<app::flows::Login> m_login_flow;
    };
}
