#include "app/flows/Login.hpp"

#include <utility>

namespace xlair::app::flows {
    Login::~Login() {
        cancel();
    }

    void Login::start(api::IClient& client, const StringView card_id) {
        cancel();
        m_client = &client;
        m_user_request = client.findUserByCard(card_id);
        if (!m_user_request) {
            fail(
                {
                    .kind = api::ErrorKind::Configuration,
                    .message = U"The API client did not create a user lookup request.",
                    .status_code = none,
                }
            );
            return;
        }
        m_state = State::FindingUser;
    }

    void Login::update(credits::CreditPool& credit_pool) {
        switch (m_state) {
            case State::FindingUser:
                updateUserLookup(credit_pool);
                break;

            case State::WaitingForCredit:
                continueWithCredit(credit_pool);
                break;

            case State::IncrementingCredits:
                updateCreditIncrement(credit_pool);
                break;

            default:
                break;
        }
    }

    void Login::cancel() {
        if (m_user_request) {
            m_user_request->cancel();
        }
        if (m_credit_request) {
            m_credit_request->cancel();
        }
        m_client = nullptr;
        m_user_request.reset();
        m_credit_request.reset();
        m_user.reset();
        m_error.reset();
        m_account_state = AccountState::Unknown;
        m_state = State::Idle;
    }

    Login::State Login::state() const noexcept {
        return m_state;
    }

    const Optional<api::User>& Login::user() const noexcept {
        return m_user;
    }

    const Optional<api::ApiError>& Login::error() const noexcept {
        return m_error;
    }

    void Login::updateUserLookup(credits::CreditPool& credit_pool) {
        if (!m_user_request) {
            return;
        }

        m_user_request->update();
        const auto& result = m_user_request->result();
        if (!result) {
            return;
        }

        if (const auto* user = std::get_if<api::User>(&*result)) {
            m_user = *user;
            m_account_state = AccountState::Registered;
        } else {
            const auto& error = std::get<api::ApiError>(*result);
            if (error.kind == api::ErrorKind::Http && error.status_code == 404) {
                m_account_state = AccountState::Unregistered;
            } else {
                fail(error);
                m_user_request.reset();
                return;
            }
        }

        m_user_request.reset();
        m_state = State::WaitingForCredit;
        continueWithCredit(credit_pool);
    }

    void Login::updateCreditIncrement(credits::CreditPool& credit_pool) {
        if (!m_credit_request) {
            return;
        }

        m_credit_request->update();
        const auto& result = m_credit_request->result();
        if (!result) {
            return;
        }

        if (const auto* credits = std::get_if<uint32>(&*result)) {
            m_user->credits = *credits;
            credit_pool.consume();
            m_state = State::UserFound;
        } else {
            fail(std::get<api::ApiError>(*result));
        }
        m_credit_request.reset();
    }

    void Login::continueWithCredit(credits::CreditPool& credit_pool) {
        if (!credit_pool.available()) {
            return;
        }

        if (m_account_state == AccountState::Unregistered) {
            m_state = State::RegistrationRequired;
            return;
        }

        if (m_account_state != AccountState::Registered || !m_user || !m_client) {
            fail(
                {
                    .kind = api::ErrorKind::Configuration,
                    .message = U"The login flow is missing the registered user or API client.",
                    .status_code = none,
                }
            );
            return;
        }

        m_credit_request = m_client->incrementCredits(m_user->id);
        if (!m_credit_request) {
            fail(
                {
                    .kind = api::ErrorKind::Configuration,
                    .message = U"The API client did not create a credit increment request.",
                    .status_code = none,
                }
            );
            return;
        }
        m_state = State::IncrementingCredits;
    }

    void Login::fail(api::ApiError error) {
        m_error = std::move(error);
        m_state = State::Failed;
    }
}
