#include "Title.hpp"

namespace xlair::ui::scenes {
    Title::Title(const InitData& init) : SceneBase{ init } {}

    void Title::draw() const {
        m_font(U"XLAIR").draw(40, 40, Palette::White);
    }
}
