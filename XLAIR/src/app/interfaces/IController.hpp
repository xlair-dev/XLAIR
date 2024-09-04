#pragma once

namespace app {
    class IController {
    public:
        virtual ~IController() = default;
        virtual void update() = 0;
    };
}
