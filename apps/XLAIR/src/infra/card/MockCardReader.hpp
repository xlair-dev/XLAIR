#pragma once

#include "app/interfaces/ICardReader.hpp"

namespace xlair::infra::card {
    class MockCardReader final : public app::interfaces::ICardReader {
    public:
        explicit MockCardReader(String card_id);

        [[nodiscard]]
        app::card::ScanRequest scan() override;

    private:
        String m_card_id;
    };
}
