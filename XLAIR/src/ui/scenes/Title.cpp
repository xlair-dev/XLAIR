#include "Title.hpp"
#include "app/consts/Version.hpp"

namespace ui {
    Title::Title(const InitData& init) : IScene(init) {
        Print << XLAIR_VERSION_STRING;
        Print << U"Title Scene";
    }
    Title::~Title() {}
    void Title::update() {
        // なにかキーを押すと遷移
        if (!Keyboard::GetAllInputs().empty()) {
            changeScene(core::types::SceneState::Login);
        }
    }
    void Title::draw() const {}
};
