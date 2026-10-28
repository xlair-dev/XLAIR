#pragma once

#include "app/config/Config.hpp"

namespace xlair::app::interfaces {
    struct ConfigSaveError {
        String message;
        FilePath path;
    };

    class IConfigWriter {
    public:
        virtual ~IConfigWriter() = default;

        // Regenerates the file with known settings; returns an error on failure.
        [[nodiscard]]
        virtual Optional<ConfigSaveError> save(const Config& saved, const Config& current) const = 0;
    };
}
