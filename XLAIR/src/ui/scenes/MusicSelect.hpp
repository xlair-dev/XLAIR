#pragma once
#include "Common.hpp"
#include "app/usecases/App.hpp"

using app::App;

namespace ui {
    class MusicSelect : public App::Scene {
    public:
        MusicSelect(const InitData& init);
        ~MusicSelect();
        void update() override;
        void draw() const override;

    private:
    };
}
