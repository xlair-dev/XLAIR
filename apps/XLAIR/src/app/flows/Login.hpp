#pragma once

#include "app/Application.hpp"

namespace xlair::app::flows {
    class Login {
    public:
        enum class State {
            Idle,
            WaitingForCard,
            CardRead,
            Failed,
        };

        explicit Login(Application& application);
        ~Login();

        Login(const Login&) = delete;
        Login& operator=(const Login&) = delete;

        void start();
        void update();
        void cancel();

        [[nodiscard]]
        State state() const noexcept;

        [[nodiscard]]
        const Optional<card::Card>& card() const noexcept;

        [[nodiscard]]
        const Optional<card::Error>& error() const noexcept;

    private:
        void fail(card::Error error);

        Application& m_application;
        card::ScanRequest m_scan;
        Optional<card::Card> m_card;
        Optional<card::Error> m_error;
        State m_state = State::Idle;
    };
}
