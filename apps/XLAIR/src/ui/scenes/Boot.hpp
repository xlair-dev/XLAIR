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
            Failed,
        };

        State m_state = State::Waiting;
    };
}
