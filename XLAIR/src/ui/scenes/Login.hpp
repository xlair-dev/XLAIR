#pragma once
#include "Common.hpp"
#include "app/usecases/App.hpp"

using app::App;

namespace ui {

    class Login : public App::Scene {
    public:
        Login(const InitData& init);
        ~Login();
        void update() override;
        void draw() const override;

    private:
    };

}
