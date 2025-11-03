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
        ControllerManager(Device device, double latency_offset = 0.0);

        ~ControllerManager();

        static bool InitializeDevice(uint32 timeout_ms = 3000);
        // static bool InitializeDeviceAsync(uint32 timeout_ms = 3000); // TODO:

        static const DeviceInput Slider(size_t index);

        static const DeviceInput Side(SideButton button);

        static std::pair<uint32, uint32> SliderTouchFrames(size_t index);

        //static std::pair<uint32, uint32> SideTouchFrames(size_t index);

        static uint64 GetLastInputTimestampMs();

        static double LatencyOffset();

        static bool SetLED(const Array<Color>& color, uint8 brightness = 0x3F);

        static inline constexpr StringView Name{ U"ControllerManager" };

    private:
        bool init() override;

        bool update() override;

        bool initializeDevice(uint32 timeout_ms);

        const DeviceInput slider(size_t index);

        const DeviceInput side(SideButton button);

        std::pair<uint32, uint32> sliderTouchFrames(size_t index);

        //std::pair<uint32, uint32> sideTouchFrames(size_t index);

        uint64 getLastInputTimestampMs() const;

        Device m_device;
        const double m_latency_offset;
        bool m_initialized = false;
        std::array<uint32, 32> m_prev_touch_frame_count{};
    };
}
