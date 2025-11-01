#include "Boot.hpp"
#include "app/consts/Version.hpp"

namespace ui {
    Boot::Boot(const InitData& init) : IScene(init) {
        Print << U"XLAIR " << XLAIR_VERSION_STRING;
    }
    Boot::~Boot() {}

    void Boot::update() {
        auto& data = getData();
        if (not m_loading) {
            Print << U"Loading sheets...";
            data.sheetRepository->loadAsync();
            m_loading = true;
            return;
        }

        if (data.sheetRepository->isRepoReady()) {
            Print << U"Loaded";
            ClearPrint();
            changeScene(app::types::SceneState::Title, 0);
        }
    }

    void Boot::draw() const {
        Scene::Rect().draw(Palette::Black);
    }
}
