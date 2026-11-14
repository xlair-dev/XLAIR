#pragma once

#include "app/interfaces/IControllerDevice.hpp"
#include <chrono>
#include <deque>
#include <string>

namespace xlair::infra::controller {
    class IPad final : public app::interfaces::IControllerDevice {
    public:
        ~IPad() override;
        app::controller::OperationResult initialize() override;
        app::controller::OperationResult update() override;
        app::controller::OperationResult setLights(const app::controller::LightFrame&) override;
        const app::controller::InputSnapshot& snapshot() const noexcept override;
    private:
        void disconnect();
        int m_socket = -1;
        std::string m_buffer;
        std::deque<std::pair<uint32, uint8>> m_pending;
        app::controller::InputSnapshot m_snapshot;
        std::chrono::steady_clock::time_point m_next_attempt{};
    };
}
