#pragma once

#include "Keyboard.hpp"
#include "app/interfaces/IControllerDevice.hpp"

#include <Siv3D.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace xlair::infra::controller {
    class V1Input {
    public:
        static constexpr std::size_t BoardCount = 4;
        static constexpr std::size_t SensorsPerBoard = 8;
        static constexpr std::size_t SensorCount = BoardCount * SensorsPerBoard;
        static constexpr std::size_t FrameLength = 4 + SensorCount;
        static constexpr std::uint64_t ValueHoldMs = 50;
        static constexpr std::uint64_t StreamTimeoutMs = 1'000;

        void reset(std::uint64_t now_ms);
        void consumeByte(std::uint8_t byte, std::uint64_t now_ms);

        [[nodiscard]]
        const std::array<std::uint8_t, SensorCount>& sensors() const noexcept;

        [[nodiscard]]
        std::uint8_t buttons() const noexcept;

        [[nodiscard]]
        bool boardAvailable(std::size_t board, std::uint64_t now_ms) const noexcept;

        [[nodiscard]]
        bool buttonsAvailable(std::uint64_t now_ms) const noexcept;

        [[nodiscard]]
        bool streamTimedOut(std::uint64_t now_ms) const noexcept;

    private:
        void finishFrame(std::uint64_t now_ms);

        std::array<std::uint8_t, FrameLength> m_frame{};
        std::size_t m_frame_size = 0;
        std::array<std::uint8_t, SensorCount> m_sensors{};
        std::uint8_t m_buttons = 0;
        std::array<std::optional<std::uint64_t>, BoardCount> m_last_valid_at{};
        std::optional<std::uint64_t> m_last_frame_at;
        std::uint64_t m_started_at = 0;
    };

    class V1 final : public app::interfaces::IControllerDevice {
    public:
        V1(StringView port, int32 baud_rate, uint8 touch_threshold);
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

        [[nodiscard]]
        Optional<uint8> touchThreshold() const noexcept override;

        void setTouchThreshold(uint8 threshold) noexcept override;

    private:
        app::controller::OperationResult openPort();
        void close();

        Serial m_serial;
        String m_port;
        int32 m_baud_rate = 115200;
        uint8 m_touch_threshold = 1;
        bool m_initialized = false;
        V1Input m_input;
        Optional<String> m_last_reconnect_error;
        app::controller::InputSnapshot m_snapshot;
        Keyboard m_keyboard;
    };
}
