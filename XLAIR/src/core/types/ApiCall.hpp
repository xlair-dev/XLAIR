#pragma once

namespace core::types {
    template<class T>
    class ApiCall {
    public:
        enum class State {
            pending,
            parsing,
            succeeded,
            failed,
            cancelled,
        };

        bool isReady() const { return m_state == State::succeeded or m_state == State::failed; }
        State state() const { return m_state; }
        const T& value() const { return m_value; }
        const String& errorMessage() const { return m_error_message; }
        void cancel();
        void update();
        
        struct Impl {
            AsyncHTTPTask http;
            Optional<AsyncTask<void>> parse;
            String raw;
        };
        explicit ApiCall(std::shared_ptr<Impl> p) : m_p(p) {}

    private:
        std::shared_ptr<Impl> m_p;
        State m_state = State::pending;
        T m_value;
        String m_error_message;
    };
}
