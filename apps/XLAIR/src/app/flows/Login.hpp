#pragma once

#include "Common.hpp"

#include <ApiClient/IClient.hpp>

namespace xlair::app::flows {
    class Login {
    public:
        enum class State {
            Idle,
            FindingUser,
            UserFound,
            UserNotFound,
            Failed,
        };

        ~Login();

        Login() = default;
        Login(const Login&) = delete;
        Login& operator=(const Login&) = delete;

        void start(api::IClient& client, StringView card_id);
        void update();
        void cancel();

        [[nodiscard]]
        State state() const noexcept;

        [[nodiscard]]
        const Optional<api::User>& user() const noexcept;

        [[nodiscard]]
        const Optional<api::ApiError>& error() const noexcept;

    private:
        void fail(api::ApiError error);

        api::Request<api::User> m_request;
        Optional<api::User> m_user;
        Optional<api::ApiError> m_error;
        State m_state = State::Idle;
    };
}
