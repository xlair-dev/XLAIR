#pragma once

#include "Common.hpp"

namespace xlair::app {
    class Application;
}

namespace xlair::ui {
    class RootView {
    public:
        RootView();

        void draw(const app::Application& application) const;

    private:
        Font m_font;
    };
}
