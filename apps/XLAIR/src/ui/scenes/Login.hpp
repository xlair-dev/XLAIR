#pragma once

#include "app/flows/Login.hpp"
#include "ui/Scene.hpp"

namespace xlair::ui::scenes {
    class Login final : public SceneBase {
    public:
        explicit Login(const InitData& init);

        void update() override;
        void draw() const override;

    private:
        void reportState() const;

        app::flows::Login m_login_flow;
    };
}
