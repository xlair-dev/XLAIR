#pragma once
#include "app/interfaces/IController.hpp"

namespace infra {
    class ControllerMock : public app::interfaces::IController {
    public:
        void update() override;
    private:
    };
}
