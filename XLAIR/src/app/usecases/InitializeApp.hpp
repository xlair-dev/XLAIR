#pragma once
#include "App.hpp"
#include "app/types/Config.hpp"

namespace app {
    void InitializeApp(App& app, const app::types::Config& config);
}
