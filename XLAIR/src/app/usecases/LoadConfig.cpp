#include "app/usecases/LoadConfig.hpp"
#include "app/consts/Config.hpp"

namespace app {
    using Config = core::types::Config;

    Config LoadConfig(const Array<std::shared_ptr<interfaces::IConfigProvider>>& providers) {
        Config result { app::consts::DefaultConfig };
        for (auto& p : providers) {
            result = p->load();
        }

        // TODO: add validation

        return result;
    }
}
