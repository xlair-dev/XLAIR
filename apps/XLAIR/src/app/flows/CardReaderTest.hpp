#pragma once

#include "app/interfaces/ICardReader.hpp"

namespace xlair::app::flows {
    class CardReaderTest {
    public:
        ~CardReaderTest();

        void start(interfaces::ICardReader* reader);
        void update();
        void stop();

        [[nodiscard]]
        bool waiting() const noexcept {
            return static_cast<bool>(m_scan);
        }

        [[nodiscard]]
        const Optional<card::Result>& result() const noexcept {
            return m_result;
        }

    private:
        card::ScanRequest m_scan;
        Optional<card::Result> m_result;
    };
}
