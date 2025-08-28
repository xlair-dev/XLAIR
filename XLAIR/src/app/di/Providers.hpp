#pragma once
#include "Common.hpp"
#include "app/interfaces/IConfigProvider.hpp"

namespace app {
    using ConfigProviderList = Array<std::unique_ptr<interfaces::IConfigProvider>>;

    ConfigProviderList MakeDefaultConfigProviders();
}
