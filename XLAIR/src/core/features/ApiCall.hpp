#pragma once

namespace core::features {
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

        struct ParseResult {
            T value{};
            bool ok = false;
        };

        using Parser = std::function<ParseResult(const JSON& json)>;

        ApiCall() = default;

        ApiCall(Method method, URLView url, const HashTable<String, String>& headers, Parser parser);
        
        [[nodiscard]]
        static ApiCall<T> MockFromFile(FilePathView path, Parser parser);

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
        State m_state = State::pending;
        T m_value;
        Parser m_parser;
        AsyncHTTPTask m_http_task;
        AsyncTask<JSON> m_mock_task;
        AsyncTask<ParseResult> m_parse_task;
        std::atomic<bool> m_cancelled{ false };

        String m_error_message;
    };
}

#include "detail/ApiCall.ipp"
