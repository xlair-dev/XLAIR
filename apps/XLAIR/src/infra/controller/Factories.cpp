#include "Factories.hpp"

#include "GroundSlider.hpp"
#include "Keyboard.hpp"
#include "V1.hpp"

namespace xlair::infra::controller {
    std::unique_ptr<app::interfaces::IControllerDevice> CreateDevice(const app::Config::Controller& config) {
        switch (config.mode) {
            case app::Config::Controller::Mode::Keyboard:
                return std::make_unique<Keyboard>();

            case app::Config::Controller::Mode::GroundSlider:
                return std::make_unique<GroundSlider>(
                    config.ground_slider.port,
                    config.ground_slider.baud_rate,
                    config.ground_slider.touch_threshold
                );

            case app::Config::Controller::Mode::V1:
                return std::make_unique<V1>(config.v1.port, config.v1.baud_rate);
        }

        return nullptr;
    }
}
