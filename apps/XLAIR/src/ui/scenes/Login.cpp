#include "Login.hpp"

namespace xlair::ui::scenes {
    Login::Login(const InitData& init) : SceneBase{ init } {
        if (const auto& card = getData().scanned_card) {
            Print << U"Card ID: " + card->card_id;
        } else {
            Print << U"Card information is not available.";
        }
    }

    void Login::update() {}

    void Login::draw() const {}
}
