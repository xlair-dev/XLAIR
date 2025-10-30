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

        ApiCall(Method method, URLView url, const HashTable<String, String>& headers, Parser parser);

        // Mock from file
        ApiCall(FilePathView path, Parser parser);

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
        AsyncTask<ParseResult<T>> m_parse_task;
        std::atomic<bool> m_cancelled{ false };

        String m_error_message;
    };
}

#include "detail/ApiCall.ipp"
