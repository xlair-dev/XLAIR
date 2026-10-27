#pragma once

#include "Base.hpp"

#include "app/flows/CardReaderTest.hpp"

namespace xlair::ui::scenes::maintenance {
    class CardReaderTest final : public Base {
    public:
        explicit CardReaderTest(const InitData& init);

        void update() override;
        void draw() const override;

    private:
        app::flows::CardReaderTest m_card_reader_test;
    };
}
