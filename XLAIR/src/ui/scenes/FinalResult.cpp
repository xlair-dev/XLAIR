#include "FinalResult.hpp"
#include "app/consts/Version.hpp"

namespace ui {
    FinalResult::FinalResult(const InitData& init) : IScene(init) {
    }
    FinalResult::~FinalResult() {}

    void FinalResult::update() {
        auto& data = getData();

        if (m_synced) {
            data.playable = data.max_playable; // reset
            data.played_records.clear();
            changeScene(app::types::SceneState::Title, 0.5s);
        }

        if (m_loading) {
            m_call.update();
            if (m_call.isReady()) {
                m_synced = true;
            }
            return;
        }

        if (not m_loading) {
            m_call = data.api->postUserRecords(data.userData.id, data.played_records);
            m_loading = true;
        }
    }

    void FinalResult::draw() const {
    }
}
