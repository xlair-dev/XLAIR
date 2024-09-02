#pragma once
#include "app/interfaces/Controller.hpp"

namespace infra {
    class ControllerMock : IController {
    public:
        void update() override;
    private:
    };
}
