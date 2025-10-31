#pragma once
#include "Common.hpp"
#include "app/usecases/App.hpp"

using app::App;

namespace ui {
    class Result : public App::Scene {
    public:
        Result(const InitData& init);
        ~Result();

        void update() override;
        void draw() const override;

    private:
    };
}
