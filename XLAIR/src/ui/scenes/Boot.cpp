#include "Boot.hpp"

namespace ui {
    Boot::Boot(const InitData& init) : IScene(init) {}
    Boot::~Boot() {}

    void Boot::update() {
        changeScene(app::types::SceneState::Title);
    }

    void Boot::draw() const {
    }
}
