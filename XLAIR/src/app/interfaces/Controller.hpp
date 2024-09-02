#pragma once

class IController {
public:
    virtual ~IController() = default;
    virtual void update() = 0;
private:
};
