#pragma once
#include "app/interfaces/ICardReader.hpp"

namespace infra::card {
    class CardReaderMock : public app::interfaces::ICardReader {
    public:
        CardReaderMock(String card_id);
        ~CardReaderMock() = default;

    private:
        const String m_card_id;
    };
}
