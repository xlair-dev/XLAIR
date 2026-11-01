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
            FetchingCatalog,
            FetchingOptions,
            UserFound,
            RegistrationRequired,
            RegisteringUser,
            ConfirmingRegistration,
            Failed,
        };

        ~Login();

        Login() = default;
        Login(const Login&) = delete;
        Login& operator=(const Login&) = delete;

        void start(api::IClient& client, StringView card_id);
        void registerUser(StringView display_name, bool is_public);
        void update(credits::CreditPool& credit_pool);
        void cancel();

        [[nodiscard]]
        bool canCancel() const noexcept;

        [[nodiscard]]
        bool newlyRegistered() const noexcept;

        [[nodiscard]]
        State state() const noexcept;

        [[nodiscard]]
        const Optional<api::User>& user() const noexcept;

        [[nodiscard]]
        const Array<api::UserRecord>& records() const noexcept;

        [[nodiscard]]
        const Optional<Array<api::Music>>& ratingCatalog() const noexcept;

        [[nodiscard]]
        const Optional<api::PlayOptions>& playOptions() const noexcept;

        [[nodiscard]]
        const Optional<api::ApiError>& error() const noexcept;

    private:
        enum class AccountState {
            Unknown,
            Registered,
            Unregistered,
        };

        void updateUserLookup(credits::CreditPool& credit_pool);
        void updateRegistration();
        void updateRegistrationConfirmation();
        void updateCreditIncrement(credits::CreditPool& credit_pool);
        void updateRecordFetch();
        void updateCatalogFetch();
        void updateOptionsFetch(credits::CreditPool& credit_pool);
        void startRecordFetch();
        void startCatalogFetch();
        void startOptionsFetch();
        void continueWithCredit(credits::CreditPool& credit_pool);
        void acceptUser(const api::User& user);
        void reset();
        void fail(api::ApiError error);

        api::IClient* m_client = nullptr;
        String m_card_id;
        api::Request<api::User> m_user_request;
        api::Request<uint32> m_credit_request;
        api::Request<Array<api::UserRecord>> m_records_request;
        api::Request<Array<api::Music>> m_catalog_request;
        api::Request<api::PlayOptions> m_options_request;
        Optional<api::User> m_user;
        Array<api::UserRecord> m_records;
        Optional<Array<api::Music>> m_rating_catalog;
        Optional<api::PlayOptions> m_play_options;
        Optional<api::ApiError> m_error;
        Optional<api::ApiError> m_registration_error;
        bool m_newly_registered = false;
        AccountState m_account_state = AccountState::Unknown;
        State m_state = State::Idle;
    };
}
