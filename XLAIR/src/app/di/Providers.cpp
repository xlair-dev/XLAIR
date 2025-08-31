#include "Providers.hpp"

// di
#include "infra/config/TomlConfigProvider.hpp"

namespace app {
    ConfigProviderList MakeDefaultConfigProviders() {
        ConfigProviderList providers;
        providers << std::make_unique<infra::config::TomlConfigProvider>(U"config.toml");
        return providers;
    }
}
