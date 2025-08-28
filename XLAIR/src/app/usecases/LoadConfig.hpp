#pragma once
#include "app/interfaces/IConfigProvider.hpp"
#include "app/types/Config.hpp"

namespace app {
    app::types::Config LoadConfig(const Array<std::unique_ptr<interfaces::IConfigProvider>>& providers);
}
