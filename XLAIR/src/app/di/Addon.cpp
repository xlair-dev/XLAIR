#include "Addon.hpp"
#include "Common.hpp"

// di
#include "ui/addons/HomographyAddon.hpp"

namespace app {
    void RegisterAddons() {
        Addon::Register<ui::addons::HomographyAddon>(ui::addons::HomographyAddon::Name);
    }
}
