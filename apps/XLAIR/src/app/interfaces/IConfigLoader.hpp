#pragma once

#include "app/config/Config.hpp"

#include <utility>

namespace xlair::app::interfaces {
    struct ConfigLoadError {
        String message;
        FilePath path;
    };

    struct ConfigLoadResult {
        Optional<Config> value;
        Optional<ConfigLoadError> error;

        explicit ConfigLoadResult(Config config) : value{ std::move(config) } {}

        explicit ConfigLoadResult(ConfigLoadError load_error) : error{ std::move(load_error) } {}

        [[nodiscard]]
        explicit operator bool() const noexcept {
            return value.has_value();
        }
    };

    class IConfigLoader {
    public:
        virtual ~IConfigLoader() = default;

        [[nodiscard]]
        virtual ConfigLoadResult load() const = 0;
    };
}
