#pragma once

#include "ui/Scene.hpp"

namespace xlair::ui::scenes {
    class Maintenance final : public SceneBase {
    public:
        explicit Maintenance(const InitData& init);
        ~Maintenance() override;

        void update() override;
        void draw() const override;

    private:
        enum class Page { Menu, Slider, Server };

        void updateSlider();
        void updateServer();
        void drawSlider() const;
        void drawServer() const;
        void back();
        bool busy() const;

        Page m_page = Page::Menu;
        std::size_t m_selected_index = 0;
        int32 m_opened_frame = Scene::FrameCount();
        bool m_sync_requested = false;
        bool m_loading_jackets = false;
    };
}
