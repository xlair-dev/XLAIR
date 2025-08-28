#include "TomlConfigProvider.hpp"
#include "app/consts/Config.hpp"

namespace infra::config {
    using Config = app::types::Config;

    void TomlConfigProvider::load(Config& config) {
        const TOMLReader toml(m_path);

        if (not toml) {
            return;
        }

        config.system.arcade = toml[U"System.arcade"].get<bool>();

        config.window.width = toml[U"Window.width"].get<int32>();
        config.window.height = toml[U"Window.height"].get<int32>();
        config.window.sizable = toml[U"Window.sizable"].get<bool>();
        config.window.fullscreen = toml[U"Window.fullscreen"].get<bool>();
        config.window.letterbox_color = ColorF(toml[U"Window.letterboxColor"].get<String>());
    };
}
