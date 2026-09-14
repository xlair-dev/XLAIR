#include <ApiClient/HttpClient.hpp>

#include "Parser.hpp"

#include <chrono>
#include <cmath>
#include <functional>

namespace xlair::api {
    using namespace s3d::Literals;
    namespace {
        using Clock = std::chrono::steady_clock;

        double Elapsed(Clock::time_point start) {
            return std::chrono::duration<double>(Clock::now() - start).count();
        }

        s3d::String UserPath(s3d::StringView user_id, s3d::StringView suffix) {
            return U"/users/" + s3d::PercentEncode(user_id) + suffix;
        }
    }

    // Shared only by requests from this client. Accessed on the application's main thread.
    struct HttpSession {
        explicit HttpSession(const ClientOptions& config)
            : endpoint{ config.endpoint }, timeout{ config.timeout_seconds }, auth{ config.auth },
              options_error{ ValidateClientOptions(config) } {
            while (endpoint.ends_with(U'/')) {
                endpoint.pop_back();
            }
        }

        s3d::URL endpoint;
        double timeout;
        AuthOptions auth;
        const s3d::Optional<OptionsError> options_error;
        s3d::String token;
        Clock::time_point token_started{};
        double token_lifetime = 0;
        s3d::Optional<ApiError> auth_error;
        std::string auth_body;
        s3d::AsyncHTTPTask auth_task;
        Clock::time_point auth_started{};

        bool hasToken() const {
            return !token.isEmpty() && Elapsed(token_started) < token_lifetime;
        }

        void updateAuthentication() {
            if (hasToken() || auth_error) {
                return;
            }
            if (!auth_task) {
                const auto& domain = auth.domain;
                const auto& client_id = auth.client_id;
                const auto& secret = auth.client_secret;
                const auto& audience = auth.audience;
                if (domain.isEmpty() || client_id.isEmpty() || secret.isEmpty() || audience.isEmpty()) {
                    auth_error = ApiError{
                        ErrorKind::Configuration,
                        U"Device authentication settings are missing.",
                        s3d::none,
                    };
                    return;
                }
                s3d::JSON body;
                body[U"client_id"] = client_id;
                body[U"client_secret"] = secret;
                body[U"audience"] = audience;
                body[U"grant_type"] = U"client_credentials";
                auth_body = body.formatUTF8();
                auth_started = Clock::now();
                const s3d::URL token_url = U"https://" + domain + U"/oauth/token";
                auth_task = s3d::SimpleHTTP::PostAsync(
                    token_url,
                    {
                        { U"Content-Type", U"application/json" },
                    },
                    auth_body.data(),
                    auth_body.size()
                );
            }

            if (Elapsed(auth_started) >= timeout) {
                auth_task.cancel();
                auth_error = ApiError{ ErrorKind::Timeout, U"Device authentication timed out.", s3d::none };
            } else if (auth_task.isReady()) {
                const auto& response = auth_task.getResponse();
                if (!response) {
                    auth_error = ApiError{
                        ErrorKind::Network,
                        U"Could not reach the authentication server.",
                        s3d::none,
                    };
                } else if (!response.isOK()) {
                    auth_error = ApiError{
                        ErrorKind::Authentication,
                        U"Device authentication failed.",
                        response.getStatusCodeInt(),
                    };
                } else {
                    const auto json = auth_task.getAsJSON();
                    const auto access_token = json.isObject() && json.hasElement(U"access_token")
                                                  ? json[U"access_token"].getOpt<s3d::String>()
                                                  : s3d::none;
                    const auto expires_in = json.isObject() && json.hasElement(U"expires_in")
                                                ? json[U"expires_in"].getOpt<double>()
                                                : s3d::none;
                    const auto type = json.isObject() && json.hasElement(U"token_type")
                                          ? json[U"token_type"].getOpt<s3d::String>()
                                          : s3d::none;
                    if (!access_token || access_token->isEmpty() || !expires_in || !std::isfinite(*expires_in) ||
                        *expires_in <= 0 || !type || type->lowercased() != U"bearer") {
                        auth_error =
                            ApiError{ ErrorKind::InvalidResponse, U"Invalid device authentication response.", 200 };
                    } else {
                        token = *access_token;
                        // Count from request start and renew early, allowing for network latency.
                        token_started = auth_started;
                        token_lifetime = *expires_in * 0.9;
                        if (!hasToken()) {
                            auth_error = ApiError{
                                ErrorKind::Authentication,
                                U"The device token has already expired.",
                                s3d::none,
                            };
                        }
                    }
                }
            } else {
                return;
            }
            auth_task = {};
            auth_body.clear();
        }
    };

    namespace {
        template <class T> class HttpRequest final : public IRequest<T> {
        public:
            using Parser = std::function<T(const s3d::JSON&)>;

            HttpRequest(
                std::shared_ptr<HttpSession> session,
                s3d::String path,
                s3d::Optional<std::string> body,
                s3d::int32 expected_status,
                Parser parser,
                s3d::Optional<s3d::FilePath> destination = s3d::none
            )
                : m_session{ std::move(session) }, m_path{ std::move(path) }, m_body{ std::move(body) },
                  m_expected_status{ expected_status }, m_parser{ std::move(parser) },
                  m_destination{ std::move(destination) } {
                // A new explicit operation may retry authentication after an earlier failure.
                m_session->auth_error.reset();
            }

            ~HttpRequest() override {
                if (!m_result && m_task) {
                    m_task.cancel();
                }
            }

            void update() override {
                if (m_result) {
                    return;
                }
                if (m_destination && (!m_path.starts_with(U'/') || m_path.starts_with(U"//") ||
                                      m_path.contains(U'\\') || m_path.contains(U'%') || m_path.contains(U"..") ||
                                      m_path.contains(U'?') || m_path.contains(U'#') || m_path.any([](char32_t ch) {
                                          return ch <= 32 || ch == 127;
                                      }) ||
                                      !(m_path.starts_with(U"/musics/") || m_path.starts_with(U"/sheets/")))) {
                    fail(ErrorKind::Configuration, U"Invalid server asset URL.");
                    return;
                }
                if (const auto& error = m_session->options_error) {
                    fail(ErrorKind::Configuration, error->message);
                    return;
                }
                if (!m_started) {
                    m_started = Clock::now();
                }
                if (Elapsed(*m_started) >= m_session->timeout) {
                    if (m_task) {
                        m_task.cancel();
                    }
                    fail(ErrorKind::Timeout, U"API request timed out.");
                    return;
                }
                try {
                    if (!m_task) {
                        m_session->updateAuthentication();
                        if (m_session->auth_error) {
                            m_result = *m_session->auth_error;
                            return;
                        }
                        if (!m_session->hasToken()) {
                            return;
                        }
                        m_token = m_session->token;
                        s3d::HashTable<s3d::String, s3d::String> headers{
                            {
                                U"Authorization",
                                U"Bearer " + m_token,
                            },
                            {
                                U"Accept",
                                U"application/json",
                            },
                        };
                        // Asset URLs are origin-relative, not relative to an API path prefix.
                        const auto origin_end =
                            m_session->endpoint.indexOf(U'/', m_session->endpoint.indexOf(U"://") + 3);
                        const s3d::URL url =
                            (m_destination ? m_session->endpoint.substr(0, origin_end) : m_session->endpoint) + m_path;
                        if (m_body) {
                            headers[U"Content-Type"] = U"application/json";
                            m_task = s3d::SimpleHTTP::PostAsync(url, headers, m_body->data(), m_body->size());
                        } else if (m_destination) {
                            headers[U"Accept"] = U"*/*";
                            m_task = s3d::SimpleHTTP::GetAsync(url, headers, *m_destination);
                        } else {
                            m_task = s3d::SimpleHTTP::GetAsync(url, headers);
                        }
                    }
                    if (!m_task.isReady()) {
                        return;
                    }
                    const auto& response = m_task.getResponse();
                    if (!response) {
                        fail(ErrorKind::Network, U"Could not reach the API server.");
                        return;
                    }
                    const s3d::int32 status = response.getStatusCodeInt();
                    if (status != m_expected_status) {
                        if (status == 401 && m_token == m_session->token) {
                            m_session->token.clear();
                        }
                        if (status == 401 && !m_body && !m_retried) {
                            m_retried = true;
                            m_task = {};
                            return;
                        }
                        // Never replay a write: an interrupted request may already have been applied.
                        fail(
                            status == 401 ? ErrorKind::Authentication : ErrorKind::Http,
                            U"API request failed (HTTP {})."_fmt(status),
                            status
                        );
                        return;
                    }
                    if constexpr (std::same_as<T, s3d::FilePath>) {
                        if (m_destination) {
                            m_result = *m_destination;
                            return;
                        }
                    }
                    const auto json = m_task.getAsJSON();
                    if (!json) {
                        fail(ErrorKind::InvalidResponse, U"The API response is not valid s3d::JSON.", status);
                        return;
                    }
                    m_result = m_parser(json);
                } catch (const s3d::Error&) {
                    // Do not propagate server bodies or credentials into user-facing logs.
                    fail(ErrorKind::InvalidResponse, U"The API response does not match the expected schema.");
                }
            }

            void cancel() override {
                if (m_result) {
                    return;
                }
                if (m_task) {
                    m_task.cancel();
                }
                fail(ErrorKind::Cancelled, U"API request cancelled.");
            }

            const s3d::Optional<Result<T>>& result() const noexcept override {
                return m_result;
            }

        private:
            void fail(ErrorKind kind, s3d::String message, s3d::Optional<s3d::int32> status = s3d::none) {
                m_result = ApiError{ kind, std::move(message), status };
            }

            std::shared_ptr<HttpSession> m_session;
            s3d::String m_path;
            s3d::Optional<std::string> m_body;
            s3d::int32 m_expected_status;
            Parser m_parser;
            s3d::Optional<s3d::FilePath> m_destination;
            bool m_retried = false;
            s3d::Optional<Clock::time_point> m_started;
            s3d::String m_token;
            s3d::AsyncHTTPTask m_task;
            s3d::Optional<Result<T>> m_result;
        };

        template <class T>
        Request<T> MakeRequest(
            const std::shared_ptr<HttpSession>& session,
            s3d::String path,
            T (*parser)(const s3d::JSON&),
            s3d::Optional<std::string> body = s3d::none,
            s3d::int32 status = 200
        ) {
            return std::make_unique<HttpRequest<T>>(session, std::move(path), std::move(body), status, parser);
        }
    }

    HttpClient::HttpClient(const ClientOptions& config) : m_session{ std::make_shared<HttpSession>(config) } {}

    Request<User> HttpClient::findUserByCard(s3d::StringView card_id) {
        return MakeRequest(m_session, U"/users?card=" + s3d::PercentEncode(card_id), ParseUser);
    }

    Request<s3d::Array<UserRecord>> HttpClient::fetchUserRecords(s3d::StringView user_id) {
        return MakeRequest(m_session, UserPath(user_id, U"/records"), ParseRecords);
    }

    Request<s3d::Array<UserRecord>>
    HttpClient::submitPlayRecords(s3d::StringView user_id, const s3d::Array<RecordSubmission>& records) {
        return MakeRequest(
            m_session,
            UserPath(user_id, U"/records"),
            ParseRecords,
            SerializeRecords(user_id, records).formatUTF8(),
            201
        );
    }

    Request<PlayOptions> HttpClient::fetchPlayOptions(s3d::StringView user_id) {
        return MakeRequest(m_session, UserPath(user_id, U"/options"), ParsePlayOptions);
    }

    Request<PlayOptions> HttpClient::savePlayOptions(s3d::StringView user_id, const PlayOptions& options) {
        return MakeRequest(
            m_session,
            UserPath(user_id, U"/options"),
            ParsePlayOptions,
            SerializePlayOptions(options).formatUTF8()
        );
    }

    Request<s3d::uint32> HttpClient::incrementCredits(s3d::StringView user_id) {
        return MakeRequest(m_session, UserPath(user_id, U"/credits/increment"), ParseCredits, std::string{});
    }

    Request<s3d::Array<Music>> HttpClient::fetchCatalog() {
        return MakeRequest(m_session, U"/sync", ParseCatalog);
    }

    Request<s3d::FilePath> HttpClient::downloadAsset(s3d::URLView url, s3d::FilePathView destination) {
        return std::make_unique<HttpRequest<s3d::FilePath>>(
            m_session,
            s3d::String{ url },
            s3d::none,
            200,
            [](const s3d::JSON&) {
                return s3d::FilePath{};
            },
            s3d::FilePath{ destination }
        );
    }
}
