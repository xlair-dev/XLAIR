#pragma once

#include "Common.hpp"

#include <array>
#include <utility>

namespace xlair::app::controller {
    inline constexpr std::size_t TouchZoneCount = 32;
    inline constexpr std::size_t SliderLightCount = TouchZoneCount - 1;
    inline constexpr std::size_t SideButtonCount = 4;
    inline constexpr std::size_t MaintenanceButtonCount = 4;

    enum class SideButton : uint8 {
        LeftUpper,
        LeftLower,
        RightUpper,
        RightLower,
    };

    enum class MaintenanceButton : uint8 {
        Button1,
        Button2,
        Button3,
        Button4,
    };

    struct InputSnapshot {
        std::array<bool, TouchZoneCount> touch_zones{};
        std::array<bool, SideButtonCount> side_buttons{};
        std::array<bool, MaintenanceButtonCount> maintenance_buttons{};

        [[nodiscard]]
        bool touchZone(const std::size_t index) const noexcept {
            return index < touch_zones.size() && touch_zones[index];
        }

        [[nodiscard]]
        bool sideButton(const SideButton button) const noexcept {
            return side_buttons[static_cast<std::size_t>(button)];
        }

        [[nodiscard]]
        bool maintenanceButton(const MaintenanceButton button) const noexcept {
            return maintenance_buttons[static_cast<std::size_t>(button)];
        }
    };

    struct LightFrame {
        // +----++----++----++----++----++----++----++----++----++----++----++----++----++----++----++----+
        // |    ||    ||    ||    ||    ||    ||    ||    ||    ||    ||    ||    ||    ||    ||    ||    |
        // |  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 |
        // |    ||    ||    ||    ||    ||    ||    ||    ||    ||    ||    ||    ||    ||    ||    ||    |
        // +----++----++----++----++----++----++----++----++----++----++----++----++----++----++----++----+
        std::array<Color, SliderLightCount> slider{};

        // +-----+                      +-----+
        // |  0  |                      |  3  |
        // +-----+-----+          +-----+-----+
        //       |  1  |          |  2  |
        //       +-----+          +-----+
        std::array<Color, SideButtonCount> side{};

        [[nodiscard]]
        Color sliderLight(const std::size_t index) const noexcept {
            return index < slider.size() ? slider[index] : Palette::Black;
        }

        [[nodiscard]]
        Color sideLight(const SideButton button) const noexcept {
            return side[static_cast<std::size_t>(button)];
        }
    };

    enum class ErrorKind {
        Unavailable,
        Connection,
        Communication,
    };

    struct Error {
        ErrorKind kind = ErrorKind::Unavailable;
        String message;
    };

    struct OperationResult {
        Optional<Error> error;

        OperationResult() = default;
        explicit OperationResult(Error operation_error) : error{ std::move(operation_error) } {}

        [[nodiscard]]
        static OperationResult makeError(const ErrorKind kind, String message) {
            return OperationResult{
                Error{
                    .kind = kind,
                    .message = std::move(message),
                },
            };
        }

        [[nodiscard]]
        explicit operator bool() const noexcept {
            return !error.has_value();
        }
    };
}
