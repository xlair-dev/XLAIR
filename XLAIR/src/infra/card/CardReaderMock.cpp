#include "CardReaderMock.hpp"

namespace infra::card {
    CardReaderMock::CardReaderMock(String card_id) : m_card_id(card_id) {}

    void CardReaderMock::startScan() {
        m_running = true;
    }

    void CardReaderMock::stopScan() {
        m_running = false;
    }

    bool CardReaderMock::isReady() const {
        return m_running and KeySpace.pressed();
    }

    bool CardReaderMock::isOK() const {
        return true;
    }

    String CardReaderMock::getID() const {
        return m_card_id;
    }
}
