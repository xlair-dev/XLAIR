#pragma once

namespace core::features {
    template<class T>
    ApiCall<T>::ApiCall(Method method, URLView url, const HashTable<String, String>& headers, Parser parser)
        :m_parser(parser) {
        // Siv3D limitation
        switch (method) {
            case Method::get: m_http_task = SimpleHTTP::GetAsync(url, headers); break;
            case Method::post: m_http_task = SimpleHTTP::PostAsync(url, headers); break;
        }
    }

    template<class T>
    ApiCall<T> ApiCall<T>::MockFromFile(FilePathView path, Parser parser) {
        auto self = ApiCall<T>{};
        self.m_parser = parser;
        self.m_mock_task = Async([&path]() {
            const JSON json = JSON::Load(path);
            return json;
        });
        return std::move(self);
    }

    template<class T>
    void ApiCall<T>::update() {
        if (m_state == State::succeeded or m_state == State::failed or m_state == State::cancelled) {
            return;
        }

        if (m_cancelled.load(std::memory_order_relaxed)) {
            m_state = State::cancelled;
            return;
        }

        if (m_state == State::pending) {
            if (m_http_task) {
                if (not m_http_task.isReady()) {
                    return;
                }
                if (m_http_task.getResponse().isOK()) {
                    m_state = State::parsing;
                    m_parse_task = Async(m_parser, m_http_task.getAsJSON());
                } else {
                    m_state = State::failed;
                }

            } else {
                if (not m_mock_task.isReady()) {
                    return;
                }
                if (m_mock_task.get().ok) {
                    m_state = State::parsing;
                    m_parse_task = Async(m_parser, m_mock_task.get());
                } else {
                    m_state = State::failed;
                }
            }
        }

        if (m_state == State::parsing) {
            if (not (m_parse_task and m_parse_task.isReady())) {
                return;
            }

            if (m_parse_task.get().ok) {
                m_state = State::succeeded;
                m_value = std::move(m_parse_task.get().value);
            } else {
                m_state = State::failed;
            }
        }
    }

    template<class T>
    void ApiCall<T>::cancel() {
        if (m_state == State::succeeded or m_state == State::failed or m_state == State::cancelled) {
            return;
        }
        m_cancelled.store(true, std::memory_order_relaxed);
        if (m_http_task) {
            m_http_task.cancel();
        }
        if (m_mock_task) {
            m_mock_task.wait();
        }
        if (m_parse_task) {
            m_parse_task.wait();
        }
        m_state = State::cancelled;
    }

    template<class T>
    bool ApiCall<T>::isReady() const {
        return m_state == State::succeeded or m_state == State::failed or State::cancelled;
    }

    template<class T>
    bool ApiCall<T>::isOK() const {
        return m_state == State::succeeded;
    }

    template<class T>
    ApiCall<T>::State ApiCall<T>::state() const {
        return m_state;
    }

    template<class T>
    const T& ApiCall<T>::value() const {
        return m_value;
    }

    template<class T>
    const String& ApiCall<T>::errorMessage() const {
        return m_error_message;
    }

}
