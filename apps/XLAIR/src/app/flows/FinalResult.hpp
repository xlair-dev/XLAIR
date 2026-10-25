#pragma once

#include "app/session/PlaySession.hpp"

#include <ApiClient/IClient.hpp>

namespace xlair::app::flows {
    // One instance owns one session's save. A failed record write must not be replayed:
    // the current API cannot distinguish a lost response from an unapplied write.
    class FinalResult {
    public:
        enum class State {
            Idle,
            SavingRecords,
            SavingOptions,
            Saved,
            Failed,
        };

        ~FinalResult();

        FinalResult() = default;
        FinalResult(const FinalResult&) = delete;
        FinalResult& operator=(const FinalResult&) = delete;

        void start(api::IClient* client, const session::PlaySession& session);
        void update();
        void retry();

        [[nodiscard]]
        State state() const noexcept;

        [[nodiscard]]
        bool canRetry() const noexcept;

        [[nodiscard]]
        const Optional<api::ApiError>& error() const noexcept;

    private:
        void saveRecords();
        void saveOptions();
        void fail(api::ApiError error);

        api::IClient* m_client = nullptr;
        String m_user_id;
        Array<api::RecordSubmission> m_records;
        api::PlayOptions m_options;
        api::Request<Array<api::UserRecord>> m_records_request;
        api::Request<api::PlayOptions> m_options_request;
        Optional<api::ApiError> m_error;
        State m_state = State::Idle;
        bool m_records_started = false;
        bool m_records_saved = false;
    };
}
