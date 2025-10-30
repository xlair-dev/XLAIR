#pragma once
#include "Common.hpp"
#include "app/usecases/App.hpp"

using app::App;

namespace ui {

    class Title : public App::Scene {
    public:
        Title(const InitData& init);
        ~Title();
        void update() override;
        void draw() const override;

    private:
        bool m_card_scanning = false;
    };

}
