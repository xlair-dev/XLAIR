#include "TomlConfigProvider.hpp"
#include "app/consts/Config.hpp"
#include "infra/sheet/SheetMock.hpp"
#include "infra/sheet/SheetProd.hpp"
#include "infra/controller/ControllerMockKeyboard.hpp"
#include "infra/controller/ControllerGroundSlider.hpp"
#include "infra/controller/ControllerV1.hpp"
#include "infra/card/CardReaderMock.hpp"
#include "infra/card/PasoriRCS3xx.hpp"
#include "infra/api/ApiClientMock.hpp"
#include "infra/api/ApiClientProd.hpp"

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
        config.system.playable = toml[U"System.playable"].getOr<int32>(3);
        config.system.menu_timer = toml[U"System.menuTimer"].getOr<int32>(90);

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
        } else if (controller_device == U"ground_slider") {
            const auto port = toml[U"Controller.port"].getOr<String>(U"COM1");
            const auto baudrate = toml[U"Controller.baudrate"].getOr<int32>(115200);
            const auto threshold = toml[U"Controller.threshold"].getOr<int8>(1);
            config.controller = std::make_shared<infra::controller::ControllerGroundSlider>(port, baudrate, threshold);
        } else if (controller_device == U"V1") {
            const auto port = toml[U"Controller.port"].getOr<String>(U"COM1");
            const auto baudrate = toml[U"Controller.baudrate"].getOr<int32>(115200);
            config.controller = std::make_shared<infra::controller::ControllerV1>(port, baudrate);
        } else {
            // TODO:
        }
        config.controller_latency_offset = toml[U"Controller.latencyOffset"].getOr<double>(0.0);

        const auto card_reader = toml[U"CardReader.device"].get<String>();
        if (card_reader == U"mock") {
            const String card_id = toml[U"CardReader.mockId"].getOr<String>(U"00000000");
            config.cardreader = std::make_shared<infra::card::CardReaderMock>(card_id);
        } else if (card_reader.starts_with(U"Pasori RC-S3")) {
            config.cardreader = std::make_shared<infra::card::PasoriRCS3xx>();
        } else {
            // Error: unsupported
        }

        const auto use_mock_api = toml[U"Api.mock"].getOr<bool>(false);
        if (use_mock_api) {
            config.api = std::make_shared<infra::api::ApiClientMock>();
        } else {
            const auto endpoint = toml[U"Api.endpoint"].get<String>();
            config.api = std::make_shared<infra::api::ApiClientProd>(endpoint);
        }

    };
}
