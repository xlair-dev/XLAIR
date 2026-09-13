#include "ui/RootView.hpp"

#include "app/Application.hpp"

namespace xlair::ui {
    RootView::RootView() : m_font{ 28 } {}

    void RootView::draw(const app::Application& application) const {
        m_font(U"XLAIR").draw(40, 40, Palette::White);
        m_font(Format(U"Data: ", application.dataDirectory())).draw(40, 128, Palette::Lightgray);
    }
}
