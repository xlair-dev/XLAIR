#include "TomlConfigProvider.hpp"
#include "app/consts/Config.hpp"
#include "infra/sheet/SheetMock.hpp"
#include "infra/sheet/SheetProd.hpp"
#include "infra/controller/ControllerMockKeyboard.hpp"
#include "infra/card/CardReaderMock.hpp"

namespace infra::config {
    using Config = app::types::Config;

    void TomlConfigProvider::load(Config& config) {
        const TOMLReader toml(m_path);

        if (not toml) {
            throw Error{ U"Failed to open config file: {}"_fmt(m_path) };
            return;
        }

        // TODO: add error handling
        config.system.arcade = toml[U"System.arcade"].get<bool>();

        config.window.width = toml[U"Window.width"].get<int32>();
        config.window.height = toml[U"Window.height"].get<int32>();
        config.window.sizable = toml[U"Window.sizable"].get<bool>();
        config.window.fullscreen = toml[U"Window.fullscreen"].get<bool>();
        config.window.letterbox_color = ColorF(toml[U"Window.letterboxColor"].get<String>());

        const bool use_mock_sheet = toml[U"Sheet.mock"].getOr<bool>(false);
        if (use_mock_sheet) {
            const size_t mock_size = toml[U"Sheet.mockSize"].getOr<size_t>(10);
            config.sheet_provider = std::make_shared<infra::sheet::SheetMock>(mock_size);
        } else {
            config.sheet_provider = std::make_shared<infra::sheet::SheetProd>();
        }

        const auto controller_device = toml[U"Controller.device"].get<String>();
        if (controller_device == U"keyboard") {
            config.controller = std::make_shared<infra::controller::ControllerMockKeyboard>();
        } else {
            // TODO:
        }

        const auto use_mock_cardreader = toml[U"CardReader.mock"].getOr<bool>(false);
        if (use_mock_cardreader) {
            const String card_id = toml[U"CardReader.mockId"].getOr<String>(U"00000000");
            config.cardreader = std::make_shared<infra::card::CardReaderMock>(card_id);
        } else {
            // TODO:
        }
    };
}
