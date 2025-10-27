#include "LoadConfig.hpp"
#include "app/consts/Config.hpp"

namespace app {
    using Config = app::types::Config;

    Config LoadConfig(const Array<std::unique_ptr<interfaces::IConfigProvider>>& providers) {
        Config config;

        for (auto& p : providers) {
            p->load(config);
        }

        // TODO: add validation
        assert(config.sheet_provider);

        return config;
    }
}
