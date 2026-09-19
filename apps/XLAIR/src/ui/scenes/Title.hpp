#pragma once

#include "app/flows/Title.hpp"
#include "ui/Scene.hpp"

namespace xlair::ui::scenes {
    class Title final : public SceneBase {
    public:
        explicit Title(const InitData& init);

        void update() override;
        void draw() const override;

    private:
        void reportState() const;

        std::unique_ptr<app::flows::Title> m_title_flow;
    };
}
