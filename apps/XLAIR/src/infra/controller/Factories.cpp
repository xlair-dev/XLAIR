#include "Factories.hpp"

#include "GroundSlider.hpp"
#include "Keyboard.hpp"

namespace xlair::infra::controller {
    std::unique_ptr<app::interfaces::IControllerDevice> CreateDevice(const app::Config::Controller& config) {
        if (config.mode == app::Config::Controller::Mode::Keyboard) {
            return std::make_unique<Keyboard>();
        }
        return std::make_unique<GroundSlider>(
            config.ground_slider.port,
            config.ground_slider.baud_rate,
            config.ground_slider.touch_threshold
        );
    }
}
