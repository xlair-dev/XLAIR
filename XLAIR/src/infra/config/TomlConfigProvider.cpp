#include "TomlConfigProvider.hpp"
#include "app/consts/Config.hpp"
#include "infra/sheet/SheetMock.hpp"

namespace infra::config {
    using Config = app::types::Config;

    void TomlConfigProvider::load(Config& config) {
        const TOMLReader toml(m_path);

        if (not toml) {
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
            // TODO
        }
    };
}
