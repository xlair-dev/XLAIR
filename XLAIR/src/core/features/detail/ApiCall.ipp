#pragma once

namespace core::features {
    template<class T>
    void ApiCall<T>::update() {
        if (!m_p) {
            return;
        }

        if (m_p->m_state == State::succeeded or m_p->m_state == State::failed or m_p->m_state == State::cancelled) {
            return;
        }

        if (m_p->m_cancelled.load(std::memory_order_relaxed)) {
            m_p->m_state = State::cancelled;
            return;
        }

        if (m_p->m_state == State::pending) {
            if (!m_p->m_is_mock) {
                // HTTP
                if (not m_p->m_http_task.isReady()) {
                    return;
                }
                if (m_p->m_http_task.getResponse().isOK()) {
                    m_p->m_state = State::parsing;
                    const JSON json = m_p->m_http_task.getAsJSON();
                    m_p->m_parse_task = Async(m_p->m_parser, json);
                } else {
                    m_p->m_error_message = U"HTTP error";
                    m_p->m_state = State::failed;
                }
            } else {
                // Mock
                if (not m_p->m_mock_task.isReady()) {
                    return;
                }
                const JSON json = m_p->m_mock_task.get();
                if (json) {
                    m_p->m_parse_task = Async(m_p->m_parser, json);
                    m_p->m_state = State::parsing;
                } else {
                    m_p->m_error_message = U"Mockdata error";
                    m_p->m_state = State::failed;
                }
            }
        }

        if (m_p->m_state == State::parsing) {
            if (not m_p->m_parse_task.isReady()) {
                return;
            }

            const auto result = m_p->m_parse_task.get();

            if (result.ok) {
                m_p->m_state = State::succeeded;
                m_p->m_value = std::move(result.value);
            } else {
                m_p->m_error_message = U"Parse error";
                m_p->m_state = State::failed;
            }
        }
    }

    template<class T>
    void ApiCall<T>::cancel() {
        if (!m_p) {
            return;
        }
        if (m_p->m_state == State::succeeded or m_p->m_state == State::failed or m_p->m_state == State::cancelled) {
            return;
        }
        m_p->m_cancelled.store(true, std::memory_order_relaxed);

        if (m_p->m_http_task) {
            m_p->m_http_task.cancel();
        }
        if (m_p->m_mock_task) {
            m_p->m_mock_task.wait();
        }
        if (m_p->m_parse_task) {
            m_p->m_parse_task.wait();
        }
        m_p->m_state = State::cancelled;
    }

    template<class T>
    bool ApiCall<T>::isReady() const {
        if (!m_p) {
            return true;
        }
        return (m_p->m_state == State::succeeded)
            or (m_p->m_state == State::failed)
            or (m_p->m_state == State::cancelled);
    }

    template<class T>
    bool ApiCall<T>::isOK() const {
        return m_p && (m_p->m_state == State::succeeded);
    }

    template<class T>
    typename ApiCall<T>::State ApiCall<T>::state() const {
        return m_p ? m_p->m_state : State::cancelled;
    }

    template<class T>
    const T& ApiCall<T>::value() const {
        return m_p->m_value;
    }

    template<class T>
    const String& ApiCall<T>::errorMessage() const {
        static const String kEmpty{};
        return m_p ? m_p->m_error_message : kEmpty;
    }
}
