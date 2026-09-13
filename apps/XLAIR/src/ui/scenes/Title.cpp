#include "Title.hpp"

namespace xlair::ui::scenes {
    Title::Title(const InitData& init) : SceneBase{ init } {}

    void Title::draw() const {
        m_font(U"XLAIR").draw(40, 40, Palette::White);
        m_font(Format(U"Data: ", getData().dataDirectory())).draw(40, 128, Palette::Lightgray);
    }
}
