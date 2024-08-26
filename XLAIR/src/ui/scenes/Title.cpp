﻿#include "Title.hpp"

namespace ui {
    Title::Title(const InitData& init) : IScene(init) {
        Print << U"Title Scene";
    }
    Title::~Title() {}
    void Title::update() {
        // なにかキーを押すと遷移
        if (!Keyboard::GetAllInputs().empty()) {
            changeScene(core::SceneState::Login);
        }
    }
    void Title::draw() const {}
};