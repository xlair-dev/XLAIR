#include "FinalResult.hpp"

#include <utility>

namespace xlair::app::flows {
    FinalResult::~FinalResult() {
        if (m_records_request) {
            m_records_request->cancel();
        }
        if (m_options_request) {
            m_options_request->cancel();
        }
    }

    void FinalResult::start(api::IClient* client, const session::PlaySession& session) {
        if (m_state != State::Idle) {
            return;
        }
        if (!client || !session.finished() || !session.user() || session.user()->id.isEmpty()) {
            fail(
                { api::ErrorKind::Configuration,
                  U"A finished session and API client are required to save results.",
                  none }
            );
            return;
        }

        m_client = client;
        m_user_id = session.user()->id;
        m_options = session.playOptions();
        m_records.reserve(session.playedRecords().size());
        for (const auto& played : session.playedRecords()) {
            // Keep repeated plays of the same sheet: each one counts on the server.
            m_records.push_back(played.record);
        }
        saveRecords();
    }

    void FinalResult::update() {
        if (m_state == State::SavingRecords) {
            m_records_request->update();
            const auto& result = m_records_request->result();
            if (!result) {
                return;
            }
            if (const auto* error = std::get_if<api::ApiError>(&*result)) {
                fail(*error);
                m_records_request.reset();
                return;
            }
            m_records_saved = true;
            m_records_request.reset();
            saveOptions();
        } else if (m_state == State::SavingOptions) {
            m_options_request->update();
            const auto& result = m_options_request->result();
            if (!result) {
                return;
            }
            if (const auto* error = std::get_if<api::ApiError>(&*result)) {
                fail(*error);
            } else {
                m_state = State::Saved;
            }
            m_options_request.reset();
        }
    }

    void FinalResult::retry() {
        if (!canRetry()) {
            return;
        }
        m_error.reset();
        if (m_records_saved) {
            saveOptions();
        } else {
            saveRecords();
        }
    }

    FinalResult::State FinalResult::state() const noexcept {
        return m_state;
    }

    bool FinalResult::canRetry() const noexcept {
        return m_state == State::Failed && m_client && (!m_records_started || m_records_saved);
    }

    const Optional<api::ApiError>& FinalResult::error() const noexcept {
        return m_error;
    }

    void FinalResult::saveRecords() {
        if (m_records.isEmpty()) {
            m_records_saved = true;
            saveOptions();
            return;
        }
        m_records_request = m_client->submitPlayRecords(m_user_id, m_records);
        if (!m_records_request) {
            fail(
                { api::ErrorKind::Configuration, U"The API client did not create a record submission request.", none }
            );
            return;
        }
        m_records_started = true;
        m_state = State::SavingRecords;
    }

    void FinalResult::saveOptions() {
        // Saving the same options replaces their values, unlike submitting records.
        m_options_request = m_client->savePlayOptions(m_user_id, m_options);
        if (!m_options_request) {
            fail(
                { api::ErrorKind::Configuration, U"The API client did not create a play-options save request.", none }
            );
            return;
        }
        m_state = State::SavingOptions;
    }

    void FinalResult::fail(api::ApiError error) {
        m_error = std::move(error);
        m_state = State::Failed;
    }
}
