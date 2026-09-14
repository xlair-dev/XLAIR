#pragma once

#include "ui/Scene.hpp"

namespace xlair::ui::scenes {
    class Boot final : public SceneBase {
    public:
        explicit Boot(const InitData& init);

        void update() override;
        void draw() const override;

    private:
        enum class State {
            Waiting,
            Loading,
            Loaded,
            WaitingForSync,
            Syncing,
            SyncFailed,
            Ready,
            Failed,
        };

        State m_state = State::Waiting;
        Stopwatch m_sync_wait;
    };
}
