#include "Keyboard.hpp"

#include <Siv3D/Keyboard.hpp>

namespace xlair::infra::controller {
    namespace {
        // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
        // |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
        // | 1 | Q | 2 | W | 3 | E | 4 | R | 5 | T | 6 | Y | 7 | U | 8 | I |
        // |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
        // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
        // |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
        // | A | Z | S | X | D | C | F | V | G | B | H | N | J | M | K | , |
        // |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |   |
        // +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
        constexpr std::array<Input, app::controller::TouchZoneCount> TouchKeys{
            // clang-format off
            Key1, KeyA,
            KeyQ, KeyZ,
            Key2, KeyS,
            KeyW, KeyX,
            Key3, KeyD,
            KeyE, KeyC,
            Key4, KeyF,
            KeyR, KeyV,

            Key5, KeyG,
            KeyT, KeyB,
            Key6, KeyH,
            KeyY, KeyN,
            Key7, KeyJ,
            KeyU, KeyM,
            Key8, KeyK,
            KeyI, KeyComma,
            // clang-format on
        };

        struct SideKeyMapping {
            app::controller::SideButton button;
            Input key;
        };

        // +-------+                            +-------+
        // |  Tab  |                            | Enter |
        // +-------+--------+          +--------+-------+
        //         | LShift |          |   BS   |
        //         +--------+          +--------+
        constexpr std::array<SideKeyMapping, app::controller::SideButtonCount> SideKeys{
            SideKeyMapping{ app::controller::SideButton::LeftUpper, KeyTab },
            SideKeyMapping{ app::controller::SideButton::LeftLower, KeyLShift },
            SideKeyMapping{ app::controller::SideButton::RightUpper, KeyBackspace },
            SideKeyMapping{ app::controller::SideButton::RightLower, KeyEnter },
        };

        struct MaintenanceKeyMapping {
            app::controller::MaintenanceButton button;
            Input key;
        };

        // F1 remains available for Siv3D's default license viewer shortcut.
        constexpr std::array<MaintenanceKeyMapping, app::controller::MaintenanceButtonCount> MaintenanceKeys{
            MaintenanceKeyMapping{ app::controller::MaintenanceButton::Button1, KeyF2 },
            MaintenanceKeyMapping{ app::controller::MaintenanceButton::Button2, KeyF3 },
            MaintenanceKeyMapping{ app::controller::MaintenanceButton::Button3, KeyF4 },
            MaintenanceKeyMapping{ app::controller::MaintenanceButton::Button4, KeyF5 },
        };
    }

    app::controller::OperationResult Keyboard::initialize() {
        m_snapshot = {};
        return {};
    }

    app::controller::OperationResult Keyboard::update() {
        for (std::size_t index = 0; index < TouchKeys.size(); ++index) {
            m_snapshot.touch_zones[index] = TouchKeys[index].pressed();
        }
        for (const auto& [button, key] : SideKeys) {
            m_snapshot.side_buttons[static_cast<std::size_t>(button)] = key.pressed();
        }
        for (const auto& [button, key] : MaintenanceKeys) {
            m_snapshot.maintenance_buttons[static_cast<std::size_t>(button)] = key.pressed();
        }
        return {};
    }

    app::controller::OperationResult Keyboard::setLights(const app::controller::LightFrame&) {
        return {};
    }

    const app::controller::InputSnapshot& Keyboard::snapshot() const noexcept {
        return m_snapshot;
    }
}
