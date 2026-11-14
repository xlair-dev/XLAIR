#include "IPad.hpp"

#include <Siv3D/Keyboard.hpp>

#if defined(__APPLE__) || defined(__linux__)
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#endif

namespace xlair::infra::controller {
    IPad::~IPad() { disconnect(); }

    void IPad::disconnect() {
#if defined(__APPLE__) || defined(__linux__)
        if (m_socket >= 0) { ::close(m_socket); }
#endif
        m_socket = -1;
        m_buffer.clear();
        m_pending.clear();
        m_snapshot = {};
        m_next_attempt = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    }

    app::controller::OperationResult IPad::initialize() {
#if defined(__APPLE__) || defined(__linux__)
        disconnect();
        m_next_attempt = {};
        return {};
#else
        return app::controller::OperationResult::makeError(
            app::controller::ErrorKind::Unavailable,
            U"The iPad controller is not supported on this platform."
        );
#endif
    }

    app::controller::OperationResult IPad::update() {
#if defined(__APPLE__) || defined(__linux__)
        if (m_socket < 0 && std::chrono::steady_clock::now() >= m_next_attempt) {
            int fd = ::socket(AF_INET, SOCK_STREAM, 0);
            if (fd >= 0) {
                sockaddr_in address{};
                address.sin_family = AF_INET;
                address.sin_port = htons(24800);
                address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
                // Connecting to a local port is normally immediate; nonblocking also keeps the game responsive.
                fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
                if (::connect(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0 || errno == EINPROGRESS) {
                    m_socket = fd;
                } else {
                    ::close(fd);
                    m_next_attempt = std::chrono::steady_clock::now() + std::chrono::seconds(1);
                }
            } else { m_next_attempt = std::chrono::steady_clock::now() + std::chrono::seconds(1); }
        }
        if (m_socket >= 0) {
            char bytes[1024];
            for (;;) {
                const auto count = ::recv(m_socket, bytes, sizeof(bytes), 0);
                if (count > 0) {
                    m_buffer.append(bytes, static_cast<std::size_t>(count));
                    if (m_buffer.size() > 16384) { disconnect(); break; }
                    std::size_t end;
                    while ((end = m_buffer.find('\n')) != std::string::npos) {
                        unsigned int slider = 0, side = 0;
                        char extra = 0;
                        const auto line = m_buffer.substr(0, end);
                        if (std::sscanf(line.c_str(), "%8x %1x%c", &slider, &side, &extra) == 2 && side < 16) {
                            if (m_pending.size() >= 512) { disconnect(); return {}; }
                            m_pending.emplace_back(slider, static_cast<uint8>(side));
                        }
                        m_buffer.erase(0, end + 1);
                    }
                } else if (count == 0 || (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR)) {
                    disconnect(); break;
                } else { break; }
            }
        }
        // Preserve each state transition, including presses and releases arriving in one frame.
        if (!m_pending.empty()) {
            const auto [slider, side] = m_pending.front();
            m_pending.pop_front();
            for (std::size_t i = 0; i < m_snapshot.touch_zones.size(); ++i) m_snapshot.touch_zones[i] = (slider & (uint32{1} << i)) != 0;
            for (std::size_t i = 0; i < m_snapshot.side_buttons.size(); ++i) m_snapshot.side_buttons[i] = (side & (uint8{1} << i)) != 0;
        }
#endif
        // Keep the same maintenance controls as the keyboard device.
        constexpr std::array<Input, app::controller::MaintenanceButtonCount> keys{ KeyF2, KeyF3, KeyF4, KeyF5 };
        for (std::size_t i = 0; i < keys.size(); ++i) {
            m_snapshot.maintenance_buttons[i] = keys[i].pressed();
        }
        return {};
    }
    app::controller::OperationResult IPad::setLights(const app::controller::LightFrame&) { return {}; }
    const app::controller::InputSnapshot& IPad::snapshot() const noexcept { return m_snapshot; }
}
