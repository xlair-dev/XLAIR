#pragma once

#include "ui/Scene.hpp"
#include "app/flows/CardReaderTest.hpp"
#include "app/flows/LightTest.hpp"

#include <span>

namespace xlair::ui::scenes {
    class Maintenance final : public SceneBase {
    public:
        explicit Maintenance(const InitData& init);
        ~Maintenance() override;

        void update() override;
        void draw() const override;

    private:
        enum class Page { Menu, Controller, Slider, Lights, CardReader, Server, Sound };

        void setPage(Page page);
        std::span<const StringView> menuItems() const;
        bool itemEnabled(std::size_t index) const;
        void updateSlider();
        void updateCardReader();
        void updateServer();
        void updateSound();
        void resetSoundRepeat();
        void drawSlider() const;
        void drawLights() const;
        void drawCardReader() const;
        void drawServer() const;
        void drawSound() const;
        void back();
        bool busy() const;

        Page m_page = Page::Menu;
        std::size_t m_selected_index = 0;
        int32 m_opened_frame = Scene::FrameCount();
        bool m_sync_requested = false;
        bool m_loading_jackets = false;
        int32 m_sound_repeat_direction = 0;
        double m_sound_hold_seconds = 0.0;
        double m_sound_next_repeat_seconds = 0.0;
        std::unique_ptr<app::flows::LightTest> m_light_test;
        app::flows::CardReaderTest m_card_reader_test;
    };
}
