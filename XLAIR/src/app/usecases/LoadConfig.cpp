#include "app/usecases/LoadConfig.hpp"
#include "app/consts/Config.hpp"

namespace app {
    using Config = app::types::Config;

    Config LoadConfig(const Array<std::shared_ptr<interfaces::IConfigProvider>>& providers) {
        Config config;

        for (auto& p : providers) {
            p->load(config);
        }

        // TODO: add validation

        return config;
    }
}
