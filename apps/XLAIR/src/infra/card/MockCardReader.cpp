#include "MockCardReader.hpp"

#include <utility>

namespace xlair::infra::card {
    namespace {
        class MockScan final : public app::card::IScan {
        public:
            explicit MockScan(String card_id) : m_card_id{ std::move(card_id) } {}

            void update() override {
                if (!m_result && KeySpace.down()) {
                    m_result = app::card::Card{ m_card_id };
                }
            }

            void cancel() override {
                if (!m_result) {
                    m_result = app::card::Error{
                        .kind = app::card::ErrorKind::Cancelled,
                        .message = U"Card scan cancelled.",
                    };
                }
            }

            const Optional<app::card::Result>& result() const noexcept override {
                return m_result;
            }

        private:
            String m_card_id;
            Optional<app::card::Result> m_result;
        };
    }

    MockCardReader::MockCardReader(String card_id) : m_card_id{ std::move(card_id) } {}

    app::card::ScanRequest MockCardReader::scan() {
        return std::make_unique<MockScan>(m_card_id);
    }
}
