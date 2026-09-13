#include "Loader.hpp"

#include <Siv3D/Color.hpp>
#include <cmath>
#include <concepts>
#include <utility>

namespace xlair::infra::config {
    namespace {
        using app::interfaces::ConfigLoadError;
        using app::interfaces::ConfigLoadResult;

        [[nodiscard]]
        ConfigLoadResult MakeError(String message, const FilePath& path) {
            return ConfigLoadResult{ ConfigLoadError{
                .message = std::move(message),
                .path = path,
            } };
        }

        template <class Type>
        [[nodiscard]]
        Optional<String> ReadValue(const TOMLReader& toml, const String& key, Type& destination) {
            if (!toml.hasMember(key)) {
                return none;
            }

            if constexpr (std::integral<Type> && !std::same_as<Type, bool> && (sizeof(Type) < sizeof(int64))) {
                const auto value = toml[key].getOpt<int64>();
                if (!value || !std::in_range<Type>(*value)) {
                    return U"Config value '{}' is invalid."_fmt(key);
                }

                destination = static_cast<Type>(*value);
            } else {
                const auto value = toml[key].getOpt<Type>();
                if (!value) {
                    return U"Config value '{}' is invalid."_fmt(key);
                }

                destination = *value;
            }

            return none;
        }

        [[nodiscard]]
        bool IsColorCode(const StringView value) {
            if (value.isEmpty() || value.front() != U'#') {
                return false;
            }

            const std::size_t digit_count = value.size() - 1;
            if (digit_count != 3 && digit_count != 4 && digit_count != 6 && digit_count != 8) {
                return false;
            }

            for (const auto character : value.substr(1)) {
                if (!IsXdigit(character)) {
                    return false;
                }
            }

            return true;
        }
    }

    Loader::Loader(FilePath path) : m_path{ std::move(path) } {}

    app::interfaces::ConfigLoadResult Loader::load() const {
        const TOMLReader toml{ m_path };
        if (!toml) {
            return MakeError(U"Failed to open or parse the config file.", m_path);
        }

        app::Config config;

        if (const auto error = ReadValue(toml, U"system.arcade", config.system.arcade)) {
            return MakeError(*error, m_path);
        }
        if (const auto error = ReadValue(toml, U"system.playable", config.system.playable)) {
            return MakeError(*error, m_path);
        }
        if (const auto error = ReadValue(toml, U"system.menu_timer_seconds", config.system.menu_timer_seconds)) {
            return MakeError(*error, m_path);
        }

        if (const auto error = ReadValue(toml, U"window.width", config.window.width)) {
            return MakeError(*error, m_path);
        }
        if (const auto error = ReadValue(toml, U"window.height", config.window.height)) {
            return MakeError(*error, m_path);
        }
        if (const auto error = ReadValue(toml, U"window.sizable", config.window.sizable)) {
            return MakeError(*error, m_path);
        }
        if (const auto error = ReadValue(toml, U"window.fullscreen", config.window.fullscreen)) {
            return MakeError(*error, m_path);
        }

        String letterbox_color;
        if (const auto error = ReadValue(toml, U"window.letterbox_color", letterbox_color)) {
            return MakeError(*error, m_path);
        }
        if (!letterbox_color.isEmpty()) {
            if (!IsColorCode(letterbox_color)) {
                return MakeError(U"Config value 'window.letterbox_color' must be a hexadecimal color code.", m_path);
            }
            config.window.letterbox_color = ColorF{ letterbox_color };
        }

        if (const auto error = ReadValue(toml, U"input.latency_offset_seconds", config.input.latency_offset_seconds)) {
            return MakeError(*error, m_path);
        }

        if (config.system.playable <= 0) {
            return MakeError(U"Config value 'system.playable' must be positive.", m_path);
        }
        if (config.system.menu_timer_seconds <= 0) {
            return MakeError(U"Config value 'system.menu_timer_seconds' must be positive.", m_path);
        }
        if (config.window.width <= 0 || config.window.height <= 0) {
            return MakeError(U"Config values 'window.width' and 'window.height' must be positive.", m_path);
        }
        if (!std::isfinite(config.input.latency_offset_seconds)) {
            return MakeError(U"Config value 'input.latency_offset_seconds' must be finite.", m_path);
        }

        return ConfigLoadResult{ std::move(config) };
    }
}
