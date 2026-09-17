#include "Factories.hpp"

#include "MockCardReader.hpp"
#include "PasoriRCS3xx.hpp"

namespace xlair::infra::card {
    std::unique_ptr<app::interfaces::ICardReader> CreateReader(const app::Config::CardReader& config) {
        if (config.mode == app::Config::CardReader::Mode::Mock) {
            return std::make_unique<MockCardReader>(config.mock.card_id);
        }
        return std::make_unique<PasoriRCS3xx>();
    }
}
