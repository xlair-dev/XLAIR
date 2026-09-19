#include "app/flows/Login.hpp"

#include <utility>

namespace xlair::app::flows {
    Login::Login(Application& application) : m_application{ application } {}

    Login::~Login() {
        cancel();
    }

    void Login::start() {
        cancel();
        m_card.reset();
        m_error.reset();

        auto* reader = m_application.cardReader();
        if (!reader) {
            fail(
                {
                    .kind = card::ErrorKind::Unavailable,
                    .message = U"A card reader is not available.",
                }
            );
            return;
        }

        m_scan = reader->scan();
        if (!m_scan) {
            fail(
                {
                    .kind = card::ErrorKind::Unavailable,
                    .message = U"Failed to start card scanning.",
                }
            );
            return;
        }

        m_state = State::WaitingForCard;
    }

    void Login::update() {
        if (m_state != State::WaitingForCard || !m_scan) {
            return;
        }

        m_scan->update();
        const auto& result = m_scan->result();
        if (!result) {
            return;
        }

        if (const auto* scanned_card = std::get_if<card::Card>(&*result)) {
            m_card = *scanned_card;
            m_state = State::CardRead;
        } else {
            fail(std::get<card::Error>(*result));
        }
        m_scan.reset();
    }

    void Login::cancel() {
        if (m_scan) {
            m_scan->cancel();
            m_scan.reset();
        }
        m_state = State::Idle;
    }

    Login::State Login::state() const noexcept {
        return m_state;
    }

    const Optional<card::Card>& Login::card() const noexcept {
        return m_card;
    }

    const Optional<card::Error>& Login::error() const noexcept {
        return m_error;
    }

    void Login::fail(card::Error error) {
        m_error = std::move(error);
        m_state = State::Failed;
    }
}
