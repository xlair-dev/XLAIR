#include "app/flows/Login.hpp"

#include <utility>

namespace xlair::app::flows {
    namespace {
        bool ShouldConfirmRegistration(const api::ApiError& error) {
            return error.kind == api::ErrorKind::Network || error.kind == api::ErrorKind::Timeout ||
                   error.kind == api::ErrorKind::InvalidResponse ||
                   (error.kind == api::ErrorKind::Http &&
                    (error.status_code == 409 || error.status_code.value_or(0) >= 500));
        }
    }

    Login::~Login() {
        reset();
    }

    void Login::start(api::IClient& client, const StringView card_id) {
        if (!canCancel()) {
            return;
        }
        cancel();
        m_client = &client;
        m_card_id = card_id;
        m_user_request = client.findUserByCard(m_card_id);
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

    void Login::registerUser(const StringView display_name, const bool is_public) {
        if (m_state != State::RegistrationRequired || !m_client) {
            return;
        }
        m_user_request = m_client->registerUser(m_card_id, display_name, is_public);
        if (!m_user_request) {
            fail(
                {
                    .kind = api::ErrorKind::Configuration,
                    .message = U"The API client did not create a user registration request.",
                    .status_code = none,
                }
            );
            return;
        }
        m_state = State::RegisteringUser;
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

            case State::FetchingRecords:
                updateRecordFetch();
                break;

            case State::FetchingCatalog:
                updateCatalogFetch();
                break;

            case State::FetchingOptions:
                updateOptionsFetch(credit_pool);
                break;

            case State::RegisteringUser:
                updateRegistration();
                break;

            case State::ConfirmingRegistration:
                updateRegistrationConfirmation();
                break;

            default:
                break;
        }
    }

    void Login::cancel() {
        if (canCancel()) {
            reset();
        }
    }

    void Login::reset() {
        if (m_user_request) {
            m_user_request->cancel();
        }
        if (m_credit_request) {
            m_credit_request->cancel();
        }
        if (m_records_request) {
            m_records_request->cancel();
        }
        if (m_catalog_request) {
            m_catalog_request->cancel();
        }
        if (m_options_request) {
            m_options_request->cancel();
        }
        m_client = nullptr;
        m_card_id.clear();
        m_user_request.reset();
        m_credit_request.reset();
        m_records_request.reset();
        m_catalog_request.reset();
        m_options_request.reset();
        m_user.reset();
        m_records.clear();
        m_rating_catalog.reset();
        m_play_options.reset();
        m_error.reset();
        m_registration_error.reset();
        m_newly_registered = false;
        m_account_state = AccountState::Unknown;
        m_state = State::Idle;
    }

    bool Login::canCancel() const noexcept {
        return m_state != State::RegisteringUser && m_state != State::ConfirmingRegistration &&
               m_state != State::IncrementingCredits;
    }

    bool Login::newlyRegistered() const noexcept {
        return m_newly_registered;
    }

    Login::State Login::state() const noexcept {
        return m_state;
    }

    const Optional<api::User>& Login::user() const noexcept {
        return m_user;
    }

    const Array<api::UserRecord>& Login::records() const noexcept {
        return m_records;
    }

    const Optional<Array<api::Music>>& Login::ratingCatalog() const noexcept {
        return m_rating_catalog;
    }

    const Optional<api::PlayOptions>& Login::playOptions() const noexcept {
        return m_play_options;
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
            acceptUser(*user);
            return;
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

    void Login::updateRegistration() {
        if (!m_user_request) {
            return;
        }

        m_user_request->update();
        const auto& result = m_user_request->result();
        if (!result) {
            return;
        }

        if (const auto* user = std::get_if<api::User>(&*result)) {
            m_newly_registered = true;
            acceptUser(*user);
            return;
        }

        const auto error = std::get<api::ApiError>(*result);
        m_user_request.reset();
        if (!ShouldConfirmRegistration(error)) {
            fail(error);
            return;
        }

        // Registration is unique by card. Read back an uncertain write instead of replaying it.
        m_registration_error = error;
        m_user_request = m_client->findUserByCard(m_card_id);
        if (!m_user_request) {
            fail(error);
            return;
        }
        m_state = State::ConfirmingRegistration;
    }

    void Login::updateRegistrationConfirmation() {
        if (!m_user_request) {
            return;
        }

        m_user_request->update();
        const auto& result = m_user_request->result();
        if (!result) {
            return;
        }

        if (const auto* user = std::get_if<api::User>(&*result)) {
            m_registration_error.reset();
            m_newly_registered = true;
            acceptUser(*user);
            return;
        }

        const auto error = std::get<api::ApiError>(*result);
        m_user_request.reset();
        if (error.kind == api::ErrorKind::Http && error.status_code == 404 && m_registration_error) {
            fail(*m_registration_error);
        } else {
            fail(error);
        }
        m_registration_error.reset();
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

    void Login::updateRecordFetch() {
        if (!m_records_request) {
            return;
        }

        m_records_request->update();
        const auto& result = m_records_request->result();
        if (!result) {
            return;
        }

        if (const auto* records = std::get_if<Array<api::UserRecord>>(&*result)) {
            m_records = *records;
            m_records_request.reset();
            startCatalogFetch();
        } else {
            fail(std::get<api::ApiError>(*result));
            m_records_request.reset();
        }
    }

    void Login::updateCatalogFetch() {
        m_catalog_request->update();
        const auto& result = m_catalog_request->result();
        if (!result) {
            return;
        }

        if (const auto* catalog = std::get_if<Array<api::Music>>(&*result)) {
            m_rating_catalog = *catalog;
        } else {
            Logger << U"[Login] Rating catalog unavailable: " + std::get<api::ApiError>(*result).message;
        }
        m_catalog_request.reset();
        startOptionsFetch();
    }

    void Login::updateOptionsFetch(credits::CreditPool& credit_pool) {
        if (!m_options_request) {
            return;
        }

        m_options_request->update();
        const auto& result = m_options_request->result();
        if (!result) {
            return;
        }

        if (const auto* options = std::get_if<api::PlayOptions>(&*result)) {
            m_play_options = *options;
            m_state = State::WaitingForCredit;
            continueWithCredit(credit_pool);
        } else {
            fail(std::get<api::ApiError>(*result));
        }
        m_options_request.reset();
    }

    void Login::startRecordFetch() {
        if (!m_client || !m_user) {
            fail(
                {
                    .kind = api::ErrorKind::Configuration,
                    .message = U"The login flow is missing the registered user or API client.",
                    .status_code = none,
                }
            );
            return;
        }

        m_records_request = m_client->fetchUserRecords(m_user->id);
        if (!m_records_request) {
            fail(
                {
                    .kind = api::ErrorKind::Configuration,
                    .message = U"The API client did not create a record lookup request.",
                    .status_code = none,
                }
            );
            return;
        }
        m_state = State::FetchingRecords;
    }

    void Login::startCatalogFetch() {
        m_rating_catalog.reset();
        m_catalog_request = m_client->fetchCatalog();
        if (!m_catalog_request) {
            Logger << U"[Login] The API client did not create a rating catalog request.";
            startOptionsFetch();
            return;
        }
        m_state = State::FetchingCatalog;
    }

    void Login::startOptionsFetch() {
        if (!m_client || !m_user) {
            fail(
                {
                    .kind = api::ErrorKind::Configuration,
                    .message = U"The login flow is missing the registered user or API client.",
                    .status_code = none,
                }
            );
            return;
        }

        // TODO: Read MIRROR here as well when the API exposes it in PlayOptions.
        m_options_request = m_client->fetchPlayOptions(m_user->id);
        if (!m_options_request) {
            fail(
                {
                    .kind = api::ErrorKind::Configuration,
                    .message = U"The API client did not create a play-options request.",
                    .status_code = none,
                }
            );
            return;
        }
        m_state = State::FetchingOptions;
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

    void Login::acceptUser(const api::User& user) {
        m_user = user;
        m_account_state = AccountState::Registered;
        m_user_request.reset();
        startRecordFetch();
    }

    void Login::fail(api::ApiError error) {
        m_error = std::move(error);
        m_state = State::Failed;
    }
}
