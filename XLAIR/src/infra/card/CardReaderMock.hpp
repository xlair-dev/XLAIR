#pragma once
#include "app/interfaces/ICardReader.hpp"

namespace infra::card {
    class CardReaderMock : public app::interfaces::ICardReader {
    public:
        CardReaderMock(String card_id);

        ~CardReaderMock() = default;

        void startScan() override;

        void stopScan() override;

        bool isReady() const override;

        bool isOK() const override;

        String getID() const override;

    private:
        const String m_card_id;
        bool m_running = false;
    };
}
