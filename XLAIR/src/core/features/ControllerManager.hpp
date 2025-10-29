#pragma once
#include "Common.hpp"
#include "app/interfaces/IController.hpp" // TODO: fix (core -> app)
#include "core/types/DeviceInput.hpp"

namespace core::features {
    class ControllerManager : public IAddon {
    private:
        using Device = std::shared_ptr<app::interfaces::IController>;
        using DeviceInput = types::DeviceInput;
        using SideButton = app::interfaces::IController::SideButton;

    public:
        ControllerManager(Device device);

        ~ControllerManager();

        static bool InitializeDevice(uint32 timeout_ms = 3000);
        // static bool InitializeDeviceAsync(uint32 timeout_ms = 3000); // TODO:

        static const DeviceInput Slider(size_t index);

        static const DeviceInput Side(SideButton button);

        static inline constexpr StringView Name{ U"ControllerManager" };

    private:
        bool init() override;

        bool update() override;

        bool initializeDevice(uint32 timeout_ms);

        const DeviceInput slider(size_t index);

        const DeviceInput side(SideButton button);

        Device m_device;
        bool m_initialized = false;
    };
}
