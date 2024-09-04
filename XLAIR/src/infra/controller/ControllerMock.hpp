#pragma once
#include "app/interfaces/IController.hpp"

namespace infra {
    class ControllerMock : app::IController {
    public:
        void update() override;
    private:
    };
}
