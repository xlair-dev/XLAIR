#include "Boot.hpp"
#include <Siv3D/Scene.hpp>

namespace xlair::ui::scenes {
    Boot::Boot(const InitData& init) : SceneBase{ init } {}

    void Boot::update() {
        // TODO: Print << U"booting..."; とかのログを出す、 (changeScene で ClearPrint する)

        changeScene(SceneState::Title, 0);
    }

    void Boot::draw() const {
        Scene::Rect().draw(Palette::Black);
    }
}
