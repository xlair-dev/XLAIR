#include "CardReaderTest.hpp"

namespace xlair::app::flows {
    CardReaderTest::~CardReaderTest() {
        stop();
    }

    void CardReaderTest::start(interfaces::ICardReader* reader) {
        stop();
        m_result.reset();
        m_scan = reader ? reader->scan() : nullptr;
        if (!m_scan) {
            m_result = card::Error{
                .kind = card::ErrorKind::Unavailable,
                .message = U"Card reader is not available.",
            };
        }
    }

    void CardReaderTest::update() {
        if (!m_scan) {
            return;
        }
        m_scan->update();
        if (const auto& result = m_scan->result()) {
            m_result = *result;
            m_scan.reset();
        }
    }

    void CardReaderTest::stop() {
        if (m_scan) {
            m_scan->cancel();
            m_scan.reset();
        }
    }
}
