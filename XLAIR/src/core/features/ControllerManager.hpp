#pragma once
#include "Common.hpp"
#include "app/interfaces/IController.hpp" // TODO: fix (core -> app)

namespace core::features {
    class ControllerManager : public IAddon {
    private:
        using Device = std::shared_ptr<app::interfaces::IController>;

    public:
        ControllerManager(Device device);

        ~ControllerManager();

        static bool InitializeDevice(uint32 timeout_ms = 3000);
        // static bool InitializeDeviceAsync(uint32 timeout_ms = 3000); // TODO:

        static inline constexpr StringView Name{ U"ControllerManager" };

    private:
        bool init() override;

        bool update() override;

        bool initializeDevice(uint32 timeout_ms);

        Device m_device;
        bool m_initialized = false;
    };
}
