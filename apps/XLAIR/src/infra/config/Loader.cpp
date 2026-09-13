#include "Loader.hpp"

#include <Siv3D/Color.hpp>
#include "infra/api/ClientOptions.hpp"
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

        class ConfigReader {
        public:
            explicit ConfigReader(const TOMLReader& toml) : m_toml{ toml } {}

            template <class Type> ConfigReader& read(const String& key, Type& destination) {
                if (!m_error) {
                    m_error = ReadValue(m_toml, key, destination);
                }
                return *this;
            }

            [[nodiscard]]
            const Optional<String>& error() const noexcept {
                return m_error;
            }

        private:
            const TOMLReader& m_toml;
            Optional<String> m_error;
        };
    }

    Loader::Loader(FilePath path) : m_path{ std::move(path) } {}

    app::interfaces::ConfigLoadResult Loader::load() const {
        const TOMLReader toml{ m_path };
        if (!toml) {
            return MakeError(U"Failed to open or parse the config file.", m_path);
        }

        ConfigReader reader{ toml };
        app::Config config;

        // reflection 使いたい
        reader
            // system
            .read(U"system.arcade", config.system.arcade)
            .read(U"system.playable", config.system.playable)
            .read(U"system.menu_timer_seconds", config.system.menu_timer_seconds)
            // window
            .read(U"window.width", config.window.width)
            .read(U"window.height", config.window.height)
            .read(U"window.sizable", config.window.sizable)
            .read(U"window.fullscreen", config.window.fullscreen)
            .read(U"window.letterbox_color", config.window.letterbox_color)
            // input
            .read(U"input.latency_offset_seconds", config.input.latency_offset_seconds)
            // api
            .read(U"api.endpoint", config.api.endpoint)
            .read(U"api.timeout_seconds", config.api.timeout_seconds)
            .read(U"api.auth.domain", config.api.auth.domain)
            .read(U"api.auth.client_id", config.api.auth.client_id)
            .read(U"api.auth.client_secret", config.api.auth.client_secret)
            .read(U"api.auth.audience", config.api.auth.audience);

        if (const auto& error = reader.error()) {
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

        if (const auto error = xlair::api::ValidateClientOptions(infra::api::ToClientOptions(config.api))) {
            return MakeError(U"Config value 'api.{}': {}"_fmt(error->field, error->message), m_path);
        }

        return ConfigLoadResult{ std::move(config) };
    }
}
