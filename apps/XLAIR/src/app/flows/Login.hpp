#pragma once

#include "Common.hpp"
#include "app/credits/CreditPool.hpp"

#include <ApiClient/IClient.hpp>

namespace xlair::app::flows {
    class Login {
    public:
        enum class State {
            Idle,
            FindingUser,
            WaitingForCredit,
            IncrementingCredits,
            FetchingRecords,
            UserFound,
            RegistrationRequired,
            Failed,
        };

        ~Login();

        Login() = default;
        Login(const Login&) = delete;
        Login& operator=(const Login&) = delete;

        void start(api::IClient& client, StringView card_id);
        void update(credits::CreditPool& credit_pool);
        void cancel();

        [[nodiscard]]
        State state() const noexcept;

        [[nodiscard]]
        const Optional<api::User>& user() const noexcept;

        [[nodiscard]]
        const Array<api::UserRecord>& records() const noexcept;

        [[nodiscard]]
        const Optional<api::ApiError>& error() const noexcept;

    private:
        enum class AccountState {
            Unknown,
            Registered,
            Unregistered,
        };

        void updateUserLookup(credits::CreditPool& credit_pool);
        void updateCreditIncrement(credits::CreditPool& credit_pool);
        void updateRecordFetch(credits::CreditPool& credit_pool);
        void startRecordFetch();
        void continueWithCredit(credits::CreditPool& credit_pool);
        void fail(api::ApiError error);

        api::IClient* m_client = nullptr;
        api::Request<api::User> m_user_request;
        api::Request<uint32> m_credit_request;
        api::Request<Array<api::UserRecord>> m_records_request;
        Optional<api::User> m_user;
        Array<api::UserRecord> m_records;
        Optional<api::ApiError> m_error;
        AccountState m_account_state = AccountState::Unknown;
        State m_state = State::Idle;
    };
}
