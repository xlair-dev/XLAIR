#include "Login.hpp"

namespace ui {
    Login::Login(const InitData& init) : IScene(init) {
        Print << U"Login Scene";
    }
    Login::~Login() {}
    void Login::update() {}
    void Login::draw() const {}
}
