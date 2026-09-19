#include "app/flows/Login.hpp"

#include <utility>

namespace xlair::app::flows {
    Login::~Login() {
        cancel();
    }

    void Login::start(api::IClient& client, const StringView card_id) {
        cancel();
        m_request = client.findUserByCard(card_id);
        if (!m_request) {
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

    void Login::update() {
        if (m_state != State::FindingUser || !m_request) {
            return;
        }

        m_request->update();
        const auto& result = m_request->result();
        if (!result) {
            return;
        }

        if (const auto* user = std::get_if<api::User>(&*result)) {
            m_user = *user;
            m_state = State::UserFound;
        } else {
            const auto& error = std::get<api::ApiError>(*result);
            if (error.kind == api::ErrorKind::Http && error.status_code == 404) {
                m_state = State::UserNotFound;
            } else {
                fail(error);
            }
        }
        m_request.reset();
    }

    void Login::cancel() {
        if (m_request) {
            m_request->cancel();
        }
        m_request.reset();
        m_user.reset();
        m_error.reset();
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

    void Login::fail(api::ApiError error) {
        m_error = std::move(error);
        m_state = State::Failed;
    }
}
