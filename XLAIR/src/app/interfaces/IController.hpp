#pragma once

namespace app::interfaces {
    class IController {
    public:
        virtual ~IController() = default;
        virtual void update() = 0;
    };
}
