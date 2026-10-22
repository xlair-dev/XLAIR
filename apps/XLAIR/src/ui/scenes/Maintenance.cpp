#include "Maintenance.hpp"

#include "ui/assets/Assets.hpp"

#include <array>

namespace xlair::ui::scenes {
    namespace {
        // Placeholder entries until the individual maintenance screens are implemented.
        constexpr std::array<StringView, 4> MenuItems{
            U"項目1",
            U"項目2",
            U"項目3",
            U"Title に戻る",
        };
    }

    Maintenance::Maintenance(const InitData& init) : SceneBase{ init } {
        auto& data = getData();
        if (data.music_select_context) {
            data.music_select_context->pause();
        }
        data.game_loader.cancel();
        ClearPrint();
    }

    void Maintenance::update() {
        // A zero-duration transition updates the new scene in the same frame as the opening button press.
        if (Scene::FrameCount() == m_opened_frame) {
            return;
        }

        if (KeyUp.down()) {
            m_selected_index = (m_selected_index + MenuItems.size() - 1) % MenuItems.size();
        }
        if (KeyDown.down()) {
            m_selected_index = (m_selected_index + 1) % MenuItems.size();
        }

        const auto* controller = getData().application->controller();
        const bool exit =
            controller && controller->maintenanceButton(app::controller::MaintenanceButton::Button1).down();
        if (exit || (KeyEnter.down() && m_selected_index == MenuItems.size() - 1)) {
            changeScene(SceneState::Title, 0);
        }
    }

    void Maintenance::draw() const {
        Scene::Rect().draw(Palette::Black);

        const FontAsset font{ assets::font::Text };
        font(U"MAINTENANCE").draw(36, Vec2{ 48, 40 }, Palette::White);
        for (std::size_t index = 0; index < MenuItems.size(); ++index) {
            const double y = 128.0 + index * 52.0;
            if (index == m_selected_index) {
                font(U"▶").draw(28, Vec2{ 48, y }, Palette::White);
            }
            font(MenuItems[index]).draw(28, Vec2{ 92, y }, Palette::White);
        }
        font(U"↑ / ↓: 選択    Enter: 決定    メンテナンス1 (F2): Title に戻る")
            .draw(20, Vec2{ 48, Scene::Height() - 56.0 }, Palette::White);
    }
}
