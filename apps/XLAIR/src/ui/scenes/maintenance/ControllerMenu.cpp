#include "ControllerMenu.hpp"

#include <array>

namespace xlair::ui::scenes::maintenance {
    namespace {
        constexpr std::array<StringView, 4> MenuItems{
            U"入力テスト",
            U"LED テスト",
            U"遅延補正",
            U"戻る",
        };
    }

    ControllerMenu::ControllerMenu(const InitData& init) : Base{ init } {}

    bool ControllerMenu::lightsSupported() const {
        const auto* controller = getData().application->controller();
        if (!controller) {
            return false;
        }
        const auto capabilities = controller->lightCapabilities();
        return capabilities.slider || capabilities.side;
    }

    void ControllerMenu::update() {
        if (!navigate(SceneState::Maintenance)) {
            return;
        }
        auto& selected = getData().maintenance_controller_index;
        const Optional<std::size_t> disabled = lightsSupported() ? none : Optional<std::size_t>{ 1 };
        UpdateMenuSelection(selected, MenuItems, disabled);
        if (!KeyEnter.down() || (disabled && selected == *disabled)) {
            return;
        }
        switch (selected) {
            case 0:
                changeScene(SceneState::MaintenanceInputTest, 0);
                break;
            case 1:
                changeScene(SceneState::MaintenanceLightTest, 0);
                break;
            case 2:
                changeScene(SceneState::MaintenanceLatencyCalibration, 0);
                break;
            default:
                changeScene(SceneState::Maintenance, 0);
                break;
        }
    }

    void ControllerMenu::draw() const {
        drawFrame(U"コントローラテスト");
        DrawMenu(
            MenuItems,
            getData().maintenance_controller_index,
            lightsSupported() ? none : Optional<std::size_t>{ 1 }
        );
    }
}
