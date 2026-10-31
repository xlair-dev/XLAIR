#include "app/flows/Title.hpp"

#include <cmath>
#include <utility>

namespace xlair::app::flows {
    Title::Title(interfaces::ICardReader& card_reader) : m_card_reader{ card_reader } {}

    Title::~Title() {
        cancel();
    }

    void Title::start() {
        cancel();
        m_card.reset();
        startScan();
    }

    void Title::startScan() {
        m_error.reset();
        m_retry_elapsed = 0.0;

        m_scan = m_card_reader.scan();
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

    void Title::update(const double delta_seconds) {
        if (m_state == State::Failed) {
            if (std::isfinite(delta_seconds) && delta_seconds > 0.0) {
                m_retry_elapsed += delta_seconds;
            }
            if (m_retry_elapsed >= RetryDelaySeconds) {
                startScan();
            }
            return;
        }
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

    void Title::cancel() {
        if (m_scan) {
            m_scan->cancel();
            m_scan.reset();
        }
        m_state = State::Idle;
        m_retry_elapsed = 0.0;
    }

    Title::State Title::state() const noexcept {
        return m_state;
    }

    const Optional<card::Card>& Title::card() const noexcept {
        return m_card;
    }

    const Optional<card::Error>& Title::error() const noexcept {
        return m_error;
    }

    void Title::fail(card::Error error) {
        m_error = std::move(error);
        m_retry_elapsed = 0.0;
        m_state = State::Failed;
    }
}
