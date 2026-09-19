#include "Factories.hpp"

#include "Keyboard.hpp"

namespace xlair::infra::controller {
    std::unique_ptr<app::interfaces::IControllerDevice> CreateDevice(const app::Config::Controller& config) {
        if (config.mode == app::Config::Controller::Mode::Keyboard) {
            return std::make_unique<Keyboard>();
        }

        // GroundSlider support will be added with its serial protocol implementation.
        return nullptr;
    }
}
