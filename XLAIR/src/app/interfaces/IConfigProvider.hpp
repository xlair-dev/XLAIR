#pragma once
#include "app/types/Config.hpp"

namespace app::interfaces {
    class IConfigProvider {
    public:
        virtual ~IConfigProvider() = default;

        virtual void load(types::Config& config) = 0;
    };
}
