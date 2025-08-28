#include "TomlConfigProvider.hpp"
#include "app/consts/Config.hpp"

namespace infra::config {
    using Config = core::types::Config;

    Config TomlConfigProvider::load() {

        Config result { app::consts::DefaultConfig };

        const TOMLReader toml(m_path);

        if (not toml) {
            return result;
        }

        result.system.arcade = toml[U"System.arcade"].get<bool>();

        result.window.width = toml[U"Window.width"].get<int32>();
        result.window.height = toml[U"Window.height"].get<int32>();
        result.window.sizable = toml[U"Window.sizable"].get<bool>();
        result.window.fullscreen = toml[U"Window.fullscreen"].get<bool>();
        result.window.letterbox_color = ColorF(toml[U"Window.letterboxColor"].get<String>());

        return result;
    };
}
