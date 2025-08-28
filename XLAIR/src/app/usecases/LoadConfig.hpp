#pragma once
#include "app/interfaces/IConfigProvider.hpp"
#include "core/types/Config.hpp"

namespace app {
    core::types::Config LoadConfig(const Array<std::shared_ptr<interfaces::IConfigProvider>>& providers);
}
