#pragma once
#include "Common.hpp"

namespace core::features {
    template<class T>
    struct ParseResult {
        T value{};
        bool ok = false;
    };
    
    template<class T>
    class ApiCall {
    public:
        enum class Method {
            get,
            post,
        };

        enum class State {
            pending,
            parsing,
            succeeded,
            failed,
            cancelled,
        };

        using Parser = std::function<ParseResult<T>(const JSON& json)>;

        ApiCall() = default;

        ApiCall(ApiCall&&) noexcept = default;
        ApiCall& operator=(ApiCall&&) noexcept = default;

        ApiCall(const ApiCall&) = delete;
        ApiCall& operator=(const ApiCall&) = delete;

        static ApiCall HTTP(Method method, URLView url, const HashTable<String, String>& headers, Parser parser) {
            ApiCall c;
            c.m_p = std::make_shared<Impl>();
            c.m_p->m_parser = std::move(parser);
            c.m_p->m_is_mock = false;

            // Siv3D limitation
            switch (method) {
                case Method::get:
                    c.m_p->m_http_task = SimpleHTTP::GetAsync(url, headers);
                    break;
                case Method::post:
                    c.m_p->m_http_task = SimpleHTTP::PostAsync(url, headers);
                    break;
            }
            return c;
        }

        static ApiCall Mock(FilePathView path, Parser parser) {
            ApiCall c;
            c.m_p = std::make_shared<Impl>();
            c.m_p->m_parser = std::move(parser);
            c.m_p->m_is_mock = true;

            c.m_p->m_mock_task = Async([path = FilePath{ path }]() {
                const JSON json = JSON::Load(path);
                return json;
            });
            return c;
        }

        void update();
        void cancel();

        [[nodiscard]]
        bool isReady() const;

        [[nodiscard]]
        bool isOK() const;

        [[nodiscard]]
        State state() const;

        [[nodiscard]]
        const T& value() const;

        [[nodiscard]]
        const String& errorMessage() const;

    private:
        struct Impl {
            State m_state = State::pending;
            T m_value{};
            Parser m_parser{};
            AsyncHTTPTask m_http_task{};
            AsyncTask<JSON> m_mock_task{};
            AsyncTask<ParseResult<T>> m_parse_task{};
            std::atomic<bool> m_cancelled{ false };

            String m_error_message{};
            bool m_is_mock = false;
        };
        std::shared_ptr<Impl> m_p;
    };
}

#include "detail/ApiCall.ipp"
