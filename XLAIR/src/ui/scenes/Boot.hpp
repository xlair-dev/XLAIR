#pragma once
#include "Common.hpp"
#include "app/usecases/App.hpp"

using app::App;

namespace ui {
    class Boot : public App::Scene {
    public:
        Boot(const InitData& init);
        ~Boot();

        void update() override;
        void draw() const override;

    private:
        bool m_loading = false;
    };
}
