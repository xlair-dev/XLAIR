#pragma once

#include "Keyboard.hpp"
#include "app/interfaces/IControllerDevice.hpp"

#include <Siv3D/Serial.hpp>

#include <array>

namespace xlair::infra::controller {
    class GroundSlider final : public app::interfaces::IControllerDevice {
    public:
        GroundSlider(StringView port, int32 baud_rate, uint8 touch_threshold);

        ~GroundSlider() override;

        GroundSlider(const GroundSlider&) = delete;

        GroundSlider& operator=(const GroundSlider&) = delete;

        [[nodiscard]]
        app::controller::OperationResult initialize() override;

        [[nodiscard]]
        app::controller::OperationResult update() override;

        [[nodiscard]]
        app::controller::OperationResult setLights(const app::controller::LightFrame& lights) override;

        [[nodiscard]]
        const app::controller::InputSnapshot& snapshot() const noexcept override;

    private:
        struct Packet {
            uint8 command = 0;
            Array<uint8> payload;
        };

        void close();

        [[nodiscard]]
        bool startInput();

        bool stopInput();

        [[nodiscard]]
        bool sendLights(const std::array<Color, app::controller::SliderLightCount>& lights);

        [[nodiscard]]
        bool sendRawCommand(const Array<uint8>& body);

        void pumpRx();

        [[nodiscard]]
        bool consumeOnePacket();

        [[nodiscard]]
        bool consumeUntilOnePacket(uint32 timeout_ms);

        Serial m_serial;
        String m_port;
        int32 m_baud_rate = 115200;
        uint8 m_touch_threshold = 1;
        bool m_initialized = false;
        bool m_input_enabled = false;
        bool m_lights_sent = false;
        Array<uint8> m_receive_buffer;
        Packet m_last_packet;
        app::controller::InputSnapshot m_snapshot;
        std::array<Color, app::controller::SliderLightCount> m_slider_lights{};
        Keyboard m_keyboard;
    };
}
