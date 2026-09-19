#pragma once

#include "Keyboard.hpp"
#include "app/interfaces/IControllerDevice.hpp"

#include <Siv3D/Serial.hpp>

#include <array>

namespace xlair::infra::controller {
    class V1 final : public app::interfaces::IControllerDevice {
    public:
        V1(StringView port, int32 baud_rate);
        ~V1() override;

        V1(const V1&) = delete;
        V1& operator=(const V1&) = delete;

        [[nodiscard]]
        app::controller::OperationResult initialize() override;

        [[nodiscard]]
        app::controller::OperationResult update() override;

        [[nodiscard]]
        app::controller::OperationResult setLights(const app::controller::LightFrame&) override;

        [[nodiscard]]
        const app::controller::InputSnapshot& snapshot() const noexcept override;

    private:
        void close();
        void consumeByte(uint8 byte);
        void finishInputFrame();

        Serial m_serial;
        String m_port;
        int32 m_baud_rate = 115200;
        bool m_initialized = false;
        bool m_input_frame_invalid = false;
        std::size_t m_input_frame_size = 0;
        std::array<bool, app::controller::TouchZoneCount> m_input_frame{};
        app::controller::InputSnapshot m_snapshot;
        Keyboard m_keyboard;
    };
}
