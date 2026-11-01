#include "ControllerInput.hpp"

namespace xlair::ui::game {
    namespace {
        constexpr std::array SideButtonMapping{
            sheets::SideButton::LeftUpper,
            sheets::SideButton::LeftLower,
            sheets::SideButton::RightUpper,
            sheets::SideButton::RightLower,
        };
    }

    void ControllerInput::read(const app::controller::Controller* controller, const int64 judged_sample) {
        *this = {};
        if (!controller) {
            return;
        }
        for (std::size_t zone = 0; zone < app::controller::TouchZoneCount; ++zone) {
            const auto& state = controller->touchZone(zone);
            if (state.pressed()) {
                pressed.slider_pressed |= uint32{ 1 } << zone;
            }
            if (state.up()) {
                touches[touch_count++] = {
                    .zone = static_cast<uint8>(zone),
                    .sample = judged_sample,
                    .phase = core::judgement::TouchEvent::Phase::Up,
                };
            }
        }
        // Releases precede presses so a swipe across sensors in one frame can complete a FLICK.
        for (std::size_t zone = 0; zone < app::controller::TouchZoneCount; ++zone) {
            if (controller->touchZone(zone).down()) {
                touches[touch_count++] = { .zone = static_cast<uint8>(zone), .sample = judged_sample };
            }
        }
        for (std::size_t button = 0; button < app::controller::SideButtonCount; ++button) {
            const auto& state = controller->sideButton(static_cast<app::controller::SideButton>(button));
            if (state.pressed()) {
                pressed.side_pressed |= uint8{ 1 } << static_cast<uint8>(SideButtonMapping[button]);
            }
            if (state.down()) {
                side_down[side_count++] = { .button = SideButtonMapping[button], .sample = judged_sample };
            }
        }
    }
}
