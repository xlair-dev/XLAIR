#pragma once
#include "core/types/Config.hpp"

namespace app::interfaces {
    class IConfigProvider {
    public:
        virtual ~IConfigProvider() = default;

        virtual core::types::Config load() = 0;
    };
}
